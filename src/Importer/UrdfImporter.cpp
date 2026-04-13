#include "apex/importer/UrdfImporter.h"

#include "apex/core/MathTypes.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <functional>


namespace apex::importer
{

    std::optional<std::string> GetEnvironmentVariableValue(const char* pName)
    {
        if (pName == nullptr || *pName == '\0')
        {
            return std::nullopt;
        }

#if defined(_WIN32)
        char* pBuffer = nullptr;
        std::size_t bufferSize = 0;
        const errno_t err = ::_dupenv_s(&pBuffer, &bufferSize, pName);
        if (err != 0 || pBuffer == nullptr)
        {
            return std::nullopt;
        }

        std::string value(pBuffer);
        std::free(pBuffer);
        return value;
#else
        const char* pValue = std::getenv(pName);
        if (pValue == nullptr)
        {
            return std::nullopt;
        }
        return std::string(pValue);
#endif
    }



    namespace
    {
        struct XacroMacro
        {
            std::vector<std::string> Parameters;
            std::string Body;
        };

        std::string Trim(std::string value)
        {
            const auto isSpace = [](unsigned char character) { return std::isspace(character) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        std::string ReadFileText(const std::filesystem::path& filePath)
        {
            std::ifstream stream(filePath);
            if (!stream)
            {
                throw std::runtime_error("Failed to open URDF/Xacro file: " + filePath.string());
            }
            return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        }

        void MaybeDumpXml(const std::string& xml)
        {
            auto envValue =  GetEnvironmentVariableValue("APEX_IMPORTER_DEBUG_DUMP");
            if (envValue.has_value())
            {
                if (const char* const dumpPath = envValue->c_str())
                {
                    std::ofstream stream(dumpPath);
                    if (stream)
                    {
                        stream << xml;
                    }
                }
            }
        }

        std::string ExtractAttribute(const std::string& text, const std::string& attributeName)
        {
            const std::regex expression(attributeName + "\\s*=\\s*\"([^\"]*)\"", std::regex::icase);
            std::smatch match;
            if (std::regex_search(text, match, expression))
            {
                return match[1].str();
            }
            return {};
        }

        std::vector<double> ParseThreeValues(const std::string& text)
        {
            std::stringstream stream(text);
            std::vector<double> values;
            double value = 0.0;
            while (stream >> value)
            {
                values.push_back(value);
            }
            return values;
        }

        apex::core::Vec3 ParseVec3OrDefault(const std::string& text, const apex::core::Vec3& fallback)
        {
            const std::vector<double> values = ParseThreeValues(text);
            if (values.size() != 3)
            {
                return fallback;
            }
            return { values[0], values[1], values[2] };
        }

        double ParseOptionalDouble(const std::string& text, double fallback)
        {
            if (text.empty())
            {
                return fallback;
            }
            return std::stod(text);
        }

        double ComputeLinkLengthFromOrigin(const std::vector<double>& origin, double fallback, bool useNorm)
        {
            if (origin.size() < 3)
            {
                return fallback;
            }
            if (useNorm)
            {
                const double length = std::sqrt(origin[0] * origin[0] + origin[1] * origin[1] + origin[2] * origin[2]);
                return length > 1e-6 ? length : fallback;
            }
            const double length = std::fabs(origin[0]);
            return length > 1e-6 ? length : fallback;
        }

        std::vector<std::string> SplitWhitespace(const std::string& text)
        {
            std::stringstream stream(text);
            std::vector<std::string> values;
            std::string token;
            while (stream >> token)
            {
                values.push_back(token);
            }
            return values;
        }

        std::filesystem::path FindExecutableInPath(const std::string& name)
        {
            auto envValue = GetEnvironmentVariableValue("PATH");
            if (!envValue.has_value())
            {
                return {};
            }
            if (const char* const pathEnv = envValue->c_str())
            {
                std::stringstream stream(pathEnv);
                std::string entry;
        #ifdef _WIN32
                const char delimiter = ';';
                const std::string executableName = name + ".exe";
        #else
                const char delimiter = ':';
                const std::string executableName = name;
        #endif
                while (std::getline(stream, entry, delimiter))
                {
                    if (entry.empty())
                    {
                        continue;
                    }
                    std::filesystem::path candidate = std::filesystem::path(entry) / executableName;
                    std::error_code ec;
                    if (std::filesystem::exists(candidate, ec))
                    {
                        return candidate;
                    }
                }
            }
            return {};
        }

        std::filesystem::path ResolveXacroExecutable()
        {
            auto envValue = GetEnvironmentVariableValue("XACRO_EXECUTABLE");
            std::string strPath;
            if (envValue.has_value())
            {
                strPath = *envValue;
            }
            if (const char* const overrideExecutable = strPath.c_str())
            {
                std::filesystem::path candidate = overrideExecutable;
                std::error_code ec;
                if (std::filesystem::exists(candidate, ec))
                {
                    return candidate;
                }
            }
            return FindExecutableInPath("xacro");
        }

        std::unordered_map<std::string, std::string> ParseAttributes(const std::string& text)
        {
            std::unordered_map<std::string, std::string> values;
            const std::regex expression(R"attr(([A-Za-z_:][A-Za-z0-9_:.-]*)\s*=\s*"([^"]*)")attr");
            for (auto it = std::sregex_iterator(text.begin(), text.end(), expression); it != std::sregex_iterator(); ++it)
            {
                values[it->str(1)] = it->str(2);
            }
            return values;
        }

        class ExpressionParser
        {
        public:
            explicit ExpressionParser(std::string text) : m_text(std::move(text)) {}

            double Parse()
            {
                m_position = 0;
                const double value = ParseExpression();
                SkipSpaces();
                if (m_position != m_text.size())
                {
                    throw std::invalid_argument("Unexpected trailing characters in expression: '" + m_text + "'.");
                }
                return value;
            }

        private:
            double ParseExpression()
            {
                double value = ParseTerm();
                while (true)
                {
                    SkipSpaces();
                    if (Match('+'))
                    {
                        value += ParseTerm();
                    }
                    else if (Match('-'))
                    {
                        value -= ParseTerm();
                    }
                    else
                    {
                        break;
                    }
                }
                return value;
            }

            double ParseTerm()
            {
                double value = ParseFactor();
                while (true)
                {
                    SkipSpaces();
                    if (Match('*'))
                    {
                        value *= ParseFactor();
                    }
                    else if (Match('/'))
                    {
                        value /= ParseFactor();
                    }
                    else
                    {
                        break;
                    }
                }
                return value;
            }

            double ParseFactor()
            {
                SkipSpaces();
                if (Match('+'))
                {
                    return ParseFactor();
                }
                if (Match('-'))
                {
                    return -ParseFactor();
                }
                if (Match('('))
                {
                    const double value = ParseExpression();
                    SkipSpaces();
                    if (!Match(')'))
                    {
                        throw std::invalid_argument("Missing ')' in expression: '" + m_text + "'.");
                    }
                    return value;
                }
                return ParseNumber();
            }

            double ParseNumber()
            {
                SkipSpaces();
                const std::size_t start = m_position;
                while (m_position < m_text.size() &&
                       (std::isdigit(static_cast<unsigned char>(m_text[m_position])) || m_text[m_position] == '.' ||
                        m_text[m_position] == 'e' || m_text[m_position] == 'E' || m_text[m_position] == '+' || m_text[m_position] == '-'))
                {
                    if ((m_text[m_position] == '+' || m_text[m_position] == '-') && m_position != start)
                    {
                        const char previous = m_text[m_position - 1];
                        if (previous != 'e' && previous != 'E')
                        {
                            break;
                        }
                    }
                    ++m_position;
                }
                if (start == m_position)
                {
                    throw std::invalid_argument("Expected numeric literal in expression: '" + m_text + "'.");
                }
                return std::stod(m_text.substr(start, m_position - start));
            }

            void SkipSpaces()
            {
                while (m_position < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_position])) != 0)
                {
                    ++m_position;
                }
            }

            bool Match(char ch)
            {
                if (m_position < m_text.size() && m_text[m_position] == ch)
                {
                    ++m_position;
                    return true;
                }
                return false;
            }

            std::string m_text;
            std::size_t m_position = 0;
        };

        std::string EvaluateExpression(const std::string& expression, const std::unordered_map<std::string, std::string>& variables)
        {
            std::string replaced = expression;
            const std::regex identifierRegex(R"(\b([A-Za-z_][A-Za-z0-9_]*)\b)");
            std::string output;
            std::size_t last = 0;
            for (auto it = std::sregex_iterator(replaced.begin(), replaced.end(), identifierRegex); it != std::sregex_iterator(); ++it)
            {
                const std::smatch match = *it;
                output.append(replaced.substr(last, static_cast<std::size_t>(match.position()) - last));
                const std::string name = match.str(1);
                const auto variableIt = variables.find(name);
                if (variableIt != variables.end())
                {
                    output.append(variableIt->second);
                }
                else if (name == "pi")
                {
                    output.append(std::to_string(apex::core::Pi()));
                }
                else
                {
                    output.append(name);
                }
                last = static_cast<std::size_t>(match.position() + match.length());
            }
            output.append(replaced.substr(last));

            const std::string trimmed = Trim(output);
            if (trimmed.empty())
            {
                return {};
            }

            const bool looksNumeric = std::regex_search(trimmed, std::regex(R"([0-9])"));
            if (!looksNumeric)
            {
                return trimmed;
            }

            try
            {
                const double value = ExpressionParser(trimmed).Parse();
                std::ostringstream builder;
                builder.setf(std::ios::fixed);
                builder.precision(6);
                builder << value;
                return Trim(builder.str());
            }
            catch (...)
            {
                return trimmed;
            }
        }

        std::string SubstituteXacroExpressions(const std::string& text, const std::unordered_map<std::string, std::string>& variables)
        {
            std::string output;
            std::size_t cursor = 0;
            while (true)
            {
                const std::size_t begin = text.find("${", cursor);
                if (begin == std::string::npos)
                {
                    output.append(text.substr(cursor));
                    break;
                }
                output.append(text.substr(cursor, begin - cursor));
                const std::size_t end = text.find('}', begin + 2);
                if (end == std::string::npos)
                {
                    output.append(text.substr(begin));
                    break;
                }
                const std::string expression = text.substr(begin + 2, end - (begin + 2));
                output.append(EvaluateExpression(expression, variables));
                cursor = end + 1;
            }
            return output;
        }


        std::string SubstituteRosStyleTokens(
            const std::string& text,
            const std::unordered_map<std::string, std::string>& xacroArgs,
            const UrdfImportOptions& options,
            const std::filesystem::path& currentDirectory);

        bool InterpretXacroConditionValue(const std::string& value)
        {
            std::string trimmed = Trim(value);
            if (trimmed.empty())
            {
                return false;
            }
            if (trimmed.size() >= 2 && ((trimmed.front() == '"' && trimmed.back() == '"') || (trimmed.front() == '\'' && trimmed.back() == '\'')))
            {
                trimmed = Trim(trimmed.substr(1, trimmed.size() - 2));
            }

            std::string lowercase = trimmed;
            std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
            if (lowercase == "true" || lowercase == "1" || lowercase == "yes" || lowercase == "on")
            {
                return true;
            }
            if (lowercase == "false" || lowercase == "0" || lowercase == "no" || lowercase == "off" || lowercase == "none")
            {
                return false;
            }
            try
            {
                return std::fabs(std::stod(trimmed)) > 1e-12;
            }
            catch (...)
            {
                return !trimmed.empty();
            }
        }

        std::string ProcessXacroConditionals(
            std::string xml,
            const std::unordered_map<std::string, std::string>& variables,
            const std::unordered_map<std::string, std::string>& xacroArgs,
            const UrdfImportOptions& options,
            const std::filesystem::path& currentDirectory)
        {
            const auto expandConditionValue = [&](const std::string& rawValue)
            {
                return SubstituteXacroExpressions(
                    SubstituteRosStyleTokens(rawValue, xacroArgs, options, currentDirectory),
                    variables);
            };

            const auto processTag = [&](const std::string& tagName, bool invert) -> bool
            {
                const std::regex tagRegex("<" + tagName + R"(\b([^>]*)>([\s\S]*?)</)" + tagName + R"(>)", std::regex::icase);
                std::string expanded;
                std::size_t last = 0;
                bool changed = false;
                for (auto it = std::sregex_iterator(xml.begin(), xml.end(), tagRegex); it != std::sregex_iterator(); ++it)
                {
                    const std::smatch match = *it;
                    expanded.append(xml.substr(last, static_cast<std::size_t>(match.position()) - last));
                    const auto attributes = ParseAttributes(match[1].str());
                    bool includeBody = false;
                    const auto valueIt = attributes.find("value");
                    if (valueIt != attributes.end())
                    {
                        includeBody = InterpretXacroConditionValue(expandConditionValue(valueIt->second));
                    }
                    if (invert)
                    {
                        includeBody = !includeBody;
                    }
                    if (includeBody)
                    {
                        expanded.append(match[2].str());
                    }
                    changed = true;
                    last = static_cast<std::size_t>(match.position() + match.length());
                }
                if (changed)
                {
                    expanded.append(xml.substr(last));
                    xml = std::move(expanded);
                }
                return changed;
            };

            for (int pass = 0; pass < 12; ++pass)
            {
                const bool changedIf = processTag("xacro:if", false);
                const bool changedUnless = processTag("xacro:unless", true);
                if (!changedIf && !changedUnless)
                {
                    break;
                }
            }
            return xml;
        }

        std::string StripXmlDeclaration(const std::string& text)
        {
            return std::regex_replace(text, std::regex(R"(<\?xml[^>]*>\s*)", std::regex::icase), std::string());
        }

        std::string StripOuterRobotElementIfPresent(const std::string& text)
        {
            const std::regex robotRegex(R"(<robot\b[^>]*>([\s\S]*?)</robot>)", std::regex::icase);
            std::smatch match;
            if (std::regex_search(text, match, robotRegex))
            {
                return match[1].str();
            }
            return text;
        }

        void AppendUniquePath(std::vector<std::filesystem::path>& values, const std::filesystem::path& path)
        {
            if (path.empty())
            {
                return;
            }
            const auto normalized = path.lexically_normal();
            if (std::find(values.begin(), values.end(), normalized) == values.end())
            {
                values.push_back(normalized);
            }
        }

        std::vector<std::filesystem::path> SplitEnvironmentPathList(const char* envName)
        {
            std::vector<std::filesystem::path> values;
            auto envValue = GetEnvironmentVariableValue(envName);
            if (!envValue.has_value())
            {
                return values;
            }
            const char* const raw = envValue->c_str();
            if (raw == nullptr || *raw == '\0')
            {
                return values;
            }
#ifdef _WIN32
            const char delimiter = ';';
#else
            const char delimiter = ':';
#endif
            std::stringstream stream(raw);
            std::string token;
            while (std::getline(stream, token, delimiter))
            {
                if (!token.empty())
                {
                    values.emplace_back(token);
                }
            }
            return values;
        }

        std::vector<std::filesystem::path> CollectPackageSearchRoots(const UrdfImportOptions& options, const std::filesystem::path& currentDirectory)
        {
            std::vector<std::filesystem::path> roots;
            AppendUniquePath(roots, currentDirectory);
            for (const auto& root : options.PackageSearchRoots)
            {
                AppendUniquePath(roots, root);
            }
            for (const auto& entry : SplitEnvironmentPathList("AMENT_PREFIX_PATH"))
            {
                AppendUniquePath(roots, entry);
                AppendUniquePath(roots, entry / "share");
            }
            for (const auto& entry : SplitEnvironmentPathList("COLCON_PREFIX_PATH"))
            {
                AppendUniquePath(roots, entry);
                AppendUniquePath(roots, entry / "share");
            }
            for (const auto& entry : SplitEnvironmentPathList("ROS_PACKAGE_PATH"))
            {
                AppendUniquePath(roots, entry);
            }
            return roots;
        }

        std::string ExtractPackageNameFromManifest(const std::filesystem::path& manifestPath)
        {
            const std::string manifestText = ReadFileText(manifestPath);
            const std::regex nameRegex(R"(<name>\s*([^<]+?)\s*</name>)", std::regex::icase);
            std::smatch match;
            if (std::regex_search(manifestText, match, nameRegex))
            {
                return Trim(match[1].str());
            }
            return {};
        }

        std::optional<std::filesystem::path> FindPackageDirectoryByManifest(
            const std::filesystem::path& searchRoot,
            const std::string& packageName)
        {
            std::error_code ec;
            if (!std::filesystem::exists(searchRoot, ec))
            {
                return std::nullopt;
            }

            std::function<std::optional<std::filesystem::path>(const std::filesystem::path&, int)> recurse;
            recurse = [&](const std::filesystem::path& directory, int depth) -> std::optional<std::filesystem::path>
            {
                if (depth > 3)
                {
                    return std::nullopt;
                }
                const std::filesystem::path manifest = directory / "package.xml";
                if (std::filesystem::exists(manifest, ec))
                {
                    if (ExtractPackageNameFromManifest(manifest) == packageName)
                    {
                        return std::filesystem::weakly_canonical(directory);
                    }
                }
                for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
                {
                    if (ec)
                    {
                        break;
                    }
                    if (entry.is_directory(ec))
                    {
                        if (const auto result = recurse(entry.path(), depth + 1))
                        {
                            return result;
                        }
                    }
                }
                return std::nullopt;
            };

            return recurse(searchRoot, 0);
        }

        std::filesystem::path ResolvePackageDirectory(
            const std::string& packageName,
            const UrdfImportOptions& options,
            const std::filesystem::path& currentDirectory)
        {
            std::error_code ec;
            for (const auto& root : CollectPackageSearchRoots(options, currentDirectory))
            {
                const std::vector<std::filesystem::path> candidates = {
                    root / packageName,
                    root / "share" / packageName
                };
                for (const auto& candidate : candidates)
                {
                    if (std::filesystem::exists(candidate, ec))
                    {
                        return std::filesystem::weakly_canonical(candidate);
                    }
                }
                if (const auto discovered = FindPackageDirectoryByManifest(root, packageName))
                {
                    return *discovered;
                }
            }
            return {};
        }

        std::string ExtractPackageNameFromUri(const std::string& uri)
        {
            const std::string prefix = "package://";
            if (uri.rfind(prefix, 0) != 0)
            {
                return {};
            }
            const std::string rest = uri.substr(prefix.size());
            const std::size_t slash = rest.find('/');
            return slash == std::string::npos ? rest : rest.substr(0, slash);
        }

        void AppendUniqueString(std::vector<std::string>& values, const std::string& value)
        {
            if (value.empty())
            {
                return;
            }
            if (std::find(values.begin(), values.end(), value) == values.end())
            {
                values.push_back(value);
            }
        }

        std::vector<std::string> CollectDescriptionPackageDependencies(
            const std::string& xml,
            const std::filesystem::path& currentDirectory)
        {
            std::vector<std::string> dependencies;

            const std::regex findRegex(R"(\$\(find\s+([A-Za-z0-9_\-]+)\))");
            for (auto it = std::sregex_iterator(xml.begin(), xml.end(), findRegex); it != std::sregex_iterator(); ++it)
            {
                AppendUniqueString(dependencies, (*it)[1].str());
            }

            const std::regex packageUriRegex(R"(package://([A-Za-z0-9_\-]+)/)");
            for (auto it = std::sregex_iterator(xml.begin(), xml.end(), packageUriRegex); it != std::sregex_iterator(); ++it)
            {
                AppendUniqueString(dependencies, (*it)[1].str());
            }

            std::error_code ec;
            std::filesystem::path cursor = currentDirectory;
            while (!cursor.empty())
            {
                const std::filesystem::path manifest = cursor / "package.xml";
                if (std::filesystem::exists(manifest, ec))
                {
                    AppendUniqueString(dependencies, ExtractPackageNameFromManifest(manifest));
                    break;
                }
                if (cursor == cursor.root_path())
                {
                    break;
                }
                cursor = cursor.parent_path();
            }

            std::sort(dependencies.begin(), dependencies.end());
            return dependencies;
        }

        std::filesystem::path ResolveUriToPath(
            const std::string& uri,
            const std::filesystem::path& currentDirectory,
            const UrdfImportOptions& options)
        {
            if (uri.empty())
            {
                return {};
            }
            const std::string packagePrefix = "package://";
            if (uri.rfind(packagePrefix, 0) == 0 && options.ResolvePackageUris)
            {
                const std::string rest = uri.substr(packagePrefix.size());
                const std::size_t slash = rest.find('/');
                const std::string packageName = slash == std::string::npos ? rest : rest.substr(0, slash);
                const std::string relative = slash == std::string::npos ? std::string() : rest.substr(slash + 1);
                const std::filesystem::path packageDir = ResolvePackageDirectory(packageName, options, currentDirectory);
                if (!packageDir.empty())
                {
                    return std::filesystem::weakly_canonical(packageDir / relative);
                }
                return {};
            }
            const std::string filePrefix = "file://";
            if (uri.rfind(filePrefix, 0) == 0)
            {
                return std::filesystem::path(uri.substr(filePrefix.size()));
            }
            std::filesystem::path path(uri);
            if (path.is_absolute())
            {
                return path;
            }
            return std::filesystem::weakly_canonical(currentDirectory / path);
        }

        std::string SubstituteRosStyleTokens(
            const std::string& text,
            const std::unordered_map<std::string, std::string>& xacroArgs,
            const UrdfImportOptions& options,
            const std::filesystem::path& currentDirectory)
        {
            std::string substituted = text;

            const std::regex argRegex(R"(\$\(arg\s+([A-Za-z_][A-Za-z0-9_]*)\))");
            std::string argSubstituted;
            std::size_t last = 0;
            for (auto it = std::sregex_iterator(substituted.begin(), substituted.end(), argRegex); it != std::sregex_iterator(); ++it)
            {
                const std::smatch match = *it;
                argSubstituted.append(substituted.substr(last, static_cast<std::size_t>(match.position()) - last));
                const auto valueIt = xacroArgs.find(match[1].str());
                if (valueIt != xacroArgs.end())
                {
                    argSubstituted.append(valueIt->second);
                }
                last = static_cast<std::size_t>(match.position() + match.length());
            }
            argSubstituted.append(substituted.substr(last));
            substituted = std::move(argSubstituted);

            const std::regex envRegex(R"(\$\(env\s+([A-Za-z_][A-Za-z0-9_]*)\))");
            std::string envSubstituted;
            last = 0;
            for (auto it = std::sregex_iterator(substituted.begin(), substituted.end(), envRegex); it != std::sregex_iterator(); ++it)
            {
                const std::smatch match = *it;
                envSubstituted.append(substituted.substr(last, static_cast<std::size_t>(match.position()) - last));

                auto envValue = GetEnvironmentVariableValue(match[1].str().c_str());
                if (envValue.has_value())
                {
                    const char* const value = envValue->c_str();
                    envSubstituted.append(value != nullptr ? std::string(value) : std::string());
                    last = static_cast<std::size_t>(match.position() + match.length());
                }
            }
            envSubstituted.append(substituted.substr(last));
            substituted = std::move(envSubstituted);

            const std::regex optenvRegex(R"(\$\(optenv\s+([A-Za-z_][A-Za-z0-9_]*)\s+([^\)]+)\))");
            std::string optenvSubstituted;
            last = 0;
            for (auto it = std::sregex_iterator(substituted.begin(), substituted.end(), optenvRegex); it != std::sregex_iterator(); ++it)
            {
                const std::smatch match = *it;
                optenvSubstituted.append(substituted.substr(last, static_cast<std::size_t>(match.position()) - last));

                auto envValue = GetEnvironmentVariableValue(match[1].str().c_str());
                if (envValue.has_value())
                {
                    const char* const value = envValue->c_str();
                    optenvSubstituted.append(value != nullptr ? std::string(value) : Trim(match[2].str()));
                    last = static_cast<std::size_t>(match.position() + match.length());
                }
            }
            optenvSubstituted.append(substituted.substr(last));
            substituted = std::move(optenvSubstituted);

            const std::regex dirnameRegex(R"(\$\(dirname\))");
            substituted = std::regex_replace(substituted, dirnameRegex, currentDirectory.generic_string());

            const std::regex findRegex(R"(\$\(find\s+([A-Za-z0-9_\-]+)\))");
            std::string findSubstituted;
            last = 0;
            for (auto it = std::sregex_iterator(substituted.begin(), substituted.end(), findRegex); it != std::sregex_iterator(); ++it)
            {
                const std::smatch match = *it;
                findSubstituted.append(substituted.substr(last, static_cast<std::size_t>(match.position()) - last));
                const std::filesystem::path packageDir = ResolvePackageDirectory(match[1].str(), options, currentDirectory);
                findSubstituted.append(packageDir.empty() ? std::string() : packageDir.generic_string());
                last = static_cast<std::size_t>(match.position() + match.length());
            }
            findSubstituted.append(substituted.substr(last));
            substituted = std::move(findSubstituted);

            std::string evalSubstituted;
            for (std::size_t index = 0; index < substituted.size();)
            {
                const std::size_t evalBegin = substituted.find("$(eval ", index);
                if (evalBegin == std::string::npos)
                {
                    evalSubstituted.append(substituted.substr(index));
                    break;
                }
                evalSubstituted.append(substituted.substr(index, evalBegin - index));
                std::size_t cursor = evalBegin + 7;
                int depth = 1;
                while (cursor < substituted.size() && depth > 0)
                {
                    if (substituted[cursor] == '(')
                    {
                        ++depth;
                    }
                    else if (substituted[cursor] == ')')
                    {
                        --depth;
                    }
                    ++cursor;
                }
                if (depth != 0)
                {
                    evalSubstituted.append(substituted.substr(evalBegin));
                    break;
                }
                const std::string expression = Trim(substituted.substr(evalBegin + 7, (cursor - 1) - (evalBegin + 7)));
                evalSubstituted.append(EvaluateExpression(expression, xacroArgs));
                index = cursor;
            }
            return evalSubstituted;
        }

        std::unordered_map<std::string, std::string> BuildXacroArgumentMap(
            const std::string& text,
            const UrdfImportOptions& options)
        {
            std::unordered_map<std::string, std::string> arguments = options.XacroArguments;
            const std::regex argRegex(R"(<xacro:arg\b([^>]*)/>)", std::regex::icase);
            for (auto it = std::sregex_iterator(text.begin(), text.end(), argRegex); it != std::sregex_iterator(); ++it)
            {
                const auto attributes = ParseAttributes((*it)[1].str());
                const auto nameIt = attributes.find("name");
                if (nameIt == attributes.end())
                {
                    continue;
                }
                if (arguments.find(nameIt->second) != arguments.end())
                {
                    continue;
                }
                const auto defaultIt = attributes.find("default");
                arguments[nameIt->second] = (defaultIt != attributes.end()) ? defaultIt->second : std::string();
            }
            return arguments;
        }

        std::string ProcessXacroIncludes(
            const std::string& text,
            const std::filesystem::path& currentDirectory,
            const UrdfImportOptions& options,
            std::set<std::filesystem::path>& includeStack,
            const std::unordered_map<std::string, std::string>& xacroArgs);

        std::string ExpandXacroMinimalRecursive(
            const std::filesystem::path& filePath,
            const UrdfImportOptions& options,
            std::set<std::filesystem::path>& includeStack)
        {
            const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(filePath);
            if (includeStack.find(canonicalPath) != includeStack.end())
            {
                throw std::runtime_error("Recursive xacro include detected: " + canonicalPath.string());
            }
            includeStack.insert(canonicalPath);

            std::string xml = ReadFileText(canonicalPath);
            xml = StripXmlDeclaration(xml);
            const auto xacroArgs = BuildXacroArgumentMap(xml, options);
            xml = SubstituteRosStyleTokens(xml, xacroArgs, options, canonicalPath.parent_path());
            xml = std::regex_replace(xml, std::regex(R"(<xacro:arg\b[^>]*/>)", std::regex::icase), std::string());
            xml = ProcessXacroIncludes(xml, canonicalPath.parent_path(), options, includeStack, xacroArgs);

            std::unordered_map<std::string, std::string> properties = xacroArgs;
            const std::regex propertyRegex(R"(<xacro:property\b([^>]*)/>)", std::regex::icase);
            for (auto it = std::sregex_iterator(xml.begin(), xml.end(), propertyRegex); it != std::sregex_iterator(); ++it)
            {
                const auto attributes = ParseAttributes((*it)[1].str());
                const auto nameIt = attributes.find("name");
                const auto valueIt = attributes.find("value");
                if (nameIt != attributes.end() && valueIt != attributes.end())
                {
                    properties[nameIt->second] = SubstituteXacroExpressions(valueIt->second, properties);
                }
            }
            xml = std::regex_replace(xml, propertyRegex, std::string());

            std::unordered_map<std::string, XacroMacro> macros;
            const std::regex macroRegex(R"(<xacro:macro\b([^>]*)>([\s\S]*?)</xacro:macro>)", std::regex::icase);
            for (auto it = std::sregex_iterator(xml.begin(), xml.end(), macroRegex); it != std::sregex_iterator(); ++it)
            {
                const auto attributes = ParseAttributes((*it)[1].str());
                const auto nameIt = attributes.find("name");
                if (nameIt == attributes.end())
                {
                    continue;
                }
                XacroMacro macro;
                const auto paramsIt = attributes.find("params");
                if (paramsIt != attributes.end())
                {
                    macro.Parameters = SplitWhitespace(paramsIt->second);
                }
                macro.Body = (*it)[2].str();
                macros[nameIt->second] = macro;
            }
            xml = std::regex_replace(xml, macroRegex, std::string());

            for (int pass = 0; pass < 8; ++pass)
            {
                bool changed = false;
                const std::regex invocationRegex(R"(<xacro:([A-Za-z_][A-Za-z0-9_:-]*)\b([^>]*)/>)", std::regex::icase);
                std::string expanded;
                std::size_t last = 0;
                for (auto it = std::sregex_iterator(xml.begin(), xml.end(), invocationRegex); it != std::sregex_iterator(); ++it)
                {
                    const std::smatch match = *it;
                    const std::string macroName = match[1].str();
                    const auto macroIt = macros.find(macroName);
                    if (macroIt == macros.end())
                    {
                        continue;
                    }
                    changed = true;
                    expanded.append(xml.substr(last, static_cast<std::size_t>(match.position()) - last));
                    std::unordered_map<std::string, std::string> variables = properties;
                    const auto arguments = ParseAttributes(match[2].str());
                    for (const auto& [key, value] : arguments)
                    {
                        variables[key] = SubstituteXacroExpressions(SubstituteRosStyleTokens(value, xacroArgs, options, canonicalPath.parent_path()), variables);
                    }
                    for (const std::string& parameter : macroIt->second.Parameters)
                    {
                        if (variables.find(parameter) == variables.end())
                        {
                            variables[parameter] = {};
                        }
                    }
                    expanded.append(SubstituteXacroExpressions(SubstituteRosStyleTokens(macroIt->second.Body, xacroArgs, options, canonicalPath.parent_path()), variables));
                    last = static_cast<std::size_t>(match.position() + match.length());
                }
                if (changed)
                {
                    expanded.append(xml.substr(last));
                    xml = std::move(expanded);
                }
                else
                {
                    break;
                }
            }

            xml = SubstituteXacroExpressions(SubstituteRosStyleTokens(xml, xacroArgs, options, canonicalPath.parent_path()), properties);
            xml = ProcessXacroConditionals(xml, properties, xacroArgs, options, canonicalPath.parent_path());
            xml = std::regex_replace(xml, std::regex(R"(xmlns:xacro\s*=\s*"[^"]*")", std::regex::icase), std::string());
            xml = std::regex_replace(xml, std::regex(R"(</?xacro:if\b[^>]*>)", std::regex::icase), std::string());
            xml = std::regex_replace(xml, std::regex(R"(</?xacro:unless\b[^>]*>)", std::regex::icase), std::string());
            xml = std::regex_replace(xml, std::regex(R"(<xacro:[A-Za-z_][A-Za-z0-9_:-]*\b[^>]*/>)", std::regex::icase), std::string());
            xml = std::regex_replace(xml, std::regex(R"(<(/?)xacro:)", std::regex::icase), "<$1");

            includeStack.erase(canonicalPath);
            return xml;
        }

        std::string ProcessXacroIncludes(
            const std::string& text,
            const std::filesystem::path& currentDirectory,
            const UrdfImportOptions& options,
            std::set<std::filesystem::path>& includeStack,
            const std::unordered_map<std::string, std::string>& xacroArgs)
        {
            const std::regex includeRegex(R"(<xacro:include\b([^>]*)/>)", std::regex::icase);
            std::string output;
            std::size_t last = 0;
            for (auto it = std::sregex_iterator(text.begin(), text.end(), includeRegex); it != std::sregex_iterator(); ++it)
            {
                const std::smatch match = *it;
                output.append(text.substr(last, static_cast<std::size_t>(match.position()) - last));
                const auto attributes = ParseAttributes(match[1].str());
                const auto filenameIt = attributes.find("filename");
                if (filenameIt == attributes.end())
                {
                    last = static_cast<std::size_t>(match.position() + match.length());
                    continue;
                }

                const std::string includeReference = SubstituteRosStyleTokens(filenameIt->second, xacroArgs, options, currentDirectory);
                std::filesystem::path includePath = ResolveUriToPath(includeReference, currentDirectory, options);
                if (includePath.empty())
                {
                    includePath = includeReference;
                }
                if (!includePath.is_absolute())
                {
                    includePath = currentDirectory / includePath;
                    std::error_code ec;
                    if (!std::filesystem::exists(includePath, ec))
                    {
                        for (const auto& extraDir : options.XacroIncludeDirectories)
                        {
                            std::filesystem::path candidate = extraDir / includeReference;
                            if (std::filesystem::exists(candidate, ec))
                            {
                                includePath = candidate;
                                break;
                            }
                        }
                    }
                }
                const std::filesystem::path canonicalInclude = std::filesystem::weakly_canonical(includePath);
                if (includeStack.find(canonicalInclude) != includeStack.end())
                {
                    throw std::runtime_error("Recursive xacro include detected: " + canonicalInclude.string());
                }
                includeStack.insert(canonicalInclude);
                std::string included = ReadFileText(canonicalInclude);
                included = StripXmlDeclaration(included);
                included = SubstituteRosStyleTokens(included, xacroArgs, options, canonicalInclude.parent_path());
                included = ProcessXacroIncludes(included, canonicalInclude.parent_path(), options, includeStack, xacroArgs);
                included = StripOuterRobotElementIfPresent(included);
                includeStack.erase(canonicalInclude);
                output.append(included);
                last = static_cast<std::size_t>(match.position() + match.length());
            }
            output.append(text.substr(last));
            return output;
        }

        std::optional<std::string> ExpandXacroExternal(const std::filesystem::path& filePath, const UrdfImportOptions& options)
        {
            const std::filesystem::path executable = ResolveXacroExecutable();
            if (executable.empty())
            {
                return std::nullopt;
            }
            const std::filesystem::path tempOutput = std::filesystem::temp_directory_path() /
                (filePath.stem().string() + std::string("_apex_expanded.urdf"));

            std::ostringstream commandBuilder;
            commandBuilder << '"' << executable.string() << '"' << " \"" << filePath.string() << "\"";
            for (const auto& [key, value] : options.XacroArguments)
            {
                commandBuilder << ' ' << key << ":=\"" << value << "\"";
            }
            commandBuilder << " -o \"" << tempOutput.string() << "\"";
            const int exitCode = std::system(commandBuilder.str().c_str());
            if (exitCode != 0)
            {
                return std::nullopt;
            }
            return ReadFileText(tempOutput);
        }

        bool LooksLikeXacro(const std::filesystem::path& filePath, const std::string& text)
        {
            return filePath.extension() == ".xacro" || text.find("xacro:") != std::string::npos;
        }

        std::vector<apex::io::MeshResource> ParseMeshResources(
            const std::string& xml,
            const std::filesystem::path& currentDirectory,
            const UrdfImportOptions& options)
        {
            std::vector<apex::io::MeshResource> meshes;
            const std::regex linkRegex(R"(<link\b([^>]*)>([\s\S]*?)</link>)", std::regex::icase);
            const std::regex visualRegex(R"(<visual\b[^>]*>([\s\S]*?)</visual>)", std::regex::icase);
            const std::regex collisionRegex(R"(<collision\b[^>]*>([\s\S]*?)</collision>)", std::regex::icase);
            const std::regex meshRegex(R"(<mesh\b([^>]*)/?>)", std::regex::icase);
            for (auto linkIt = std::sregex_iterator(xml.begin(), xml.end(), linkRegex); linkIt != std::sregex_iterator(); ++linkIt)
            {
                const std::string linkAttributes = (*linkIt)[1].str();
                const std::string linkBody = (*linkIt)[2].str();
                const std::string linkName = ExtractAttribute(linkAttributes, "name");

                const auto collectMeshes = [&](const std::regex& sectionRegex, const std::string& role)
                {
                    for (auto sectionIt = std::sregex_iterator(linkBody.begin(), linkBody.end(), sectionRegex); sectionIt != std::sregex_iterator(); ++sectionIt)
                    {
                        const std::string sectionBody = (*sectionIt)[1].str();
                        std::smatch meshMatch;
                        if (std::regex_search(sectionBody, meshMatch, meshRegex))
                        {
                            apex::io::MeshResource resource;
                            resource.LinkName = linkName;
                            resource.Role = role;
                            resource.Uri = ExtractAttribute(meshMatch[1].str(), "filename");
                            resource.PackageName = ExtractPackageNameFromUri(resource.Uri);
                            const std::filesystem::path resolvedPath = ResolveUriToPath(resource.Uri, currentDirectory, options);
                            if (!resolvedPath.empty())
                            {
                                resource.ResolvedPath = resolvedPath.string();
                            }
                            resource.Scale = ParseVec3OrDefault(ExtractAttribute(meshMatch[1].str(), "scale"), {1.0, 1.0, 1.0});
                            if (!resource.Uri.empty())
                            {
                                meshes.push_back(resource);
                            }
                        }
                    }
                };

                collectMeshes(visualRegex, "visual_mesh");
                collectMeshes(collisionRegex, "collision_mesh");
            }
            return meshes;
        }
    }

        std::string ExpandXacroMacroBodiesFallback(
            const std::string& text,
            const std::unordered_map<std::string, std::string>& properties)
        {
            std::unordered_map<std::string, XacroMacro> macros;
            const std::regex macroRegex(R"(<xacro:macro\b([^>]*)>([\s\S]*?)</xacro:macro>)", std::regex::icase);
            for (auto it = std::sregex_iterator(text.begin(), text.end(), macroRegex); it != std::sregex_iterator(); ++it)
            {
                const auto attributes = ParseAttributes((*it)[1].str());
                const auto nameIt = attributes.find("name");
                if (nameIt == attributes.end())
                {
                    continue;
                }
                XacroMacro macro;
                const auto paramsIt = attributes.find("params");
                if (paramsIt != attributes.end())
                {
                    macro.Parameters = SplitWhitespace(paramsIt->second);
                }
                macro.Body = (*it)[2].str();
                macros[nameIt->second] = macro;
            }

            const std::regex invocationRegex(R"(<xacro:([A-Za-z_][A-Za-z0-9_:-]*)\b([^>]*)/>)", std::regex::icase);
            std::ostringstream builder;
            for (auto it = std::sregex_iterator(text.begin(), text.end(), invocationRegex); it != std::sregex_iterator(); ++it)
            {
                const std::string macroName = (*it)[1].str();
                const auto macroIt = macros.find(macroName);
                if (macroIt == macros.end())
                {
                    continue;
                }
                std::unordered_map<std::string, std::string> variables = properties;
                const auto arguments = ParseAttributes((*it)[2].str());
                for (const auto& [key, value] : arguments)
                {
                    variables[key] = SubstituteXacroExpressions(value, variables);
                }
                builder << SubstituteXacroExpressions(macroIt->second.Body, variables) << "\n";
            }
            return builder.str();
        }

    UrdfImportResult UrdfImporter::ImportFromFile(const std::filesystem::path& filePath, const UrdfImportOptions& options) const
    {
        std::filesystem::path absolutePath = std::filesystem::absolute(filePath);
        std::string sourceText = ReadFileText(absolutePath);

        UrdfImportResult result;
        result.UsedXacro = LooksLikeXacro(absolutePath, sourceText);

        std::string xml = sourceText;
        if (result.UsedXacro && options.ResolveXacro)
        {
            if (options.PreferExternalXacroTool)
            {
                if (options.XacroArguments.empty())
                {
                    if (const auto external = ExpandXacroExternal(absolutePath, options))
                {
                        xml = *external;
                        result.UsedExternalXacroTool = true;
                    }
                    else
                    {
                        std::set<std::filesystem::path> includeStack;
                        xml = ExpandXacroMinimalRecursive(absolutePath, options, includeStack);
                    }
                }
                else
                {
                    std::set<std::filesystem::path> includeStack;
                    xml = ExpandXacroMinimalRecursive(absolutePath, options, includeStack);
                }
            }
            else
            {
                std::set<std::filesystem::path> includeStack;
                xml = ExpandXacroMinimalRecursive(absolutePath, options, includeStack);
            }
        }
        if (result.UsedXacro)
        {
            std::set<std::filesystem::path> includeStack;
            includeStack.insert(std::filesystem::weakly_canonical(absolutePath));
            std::string combinedXacro = StripXmlDeclaration(sourceText);
            const auto xacroArgs = BuildXacroArgumentMap(combinedXacro, options);
            combinedXacro = SubstituteRosStyleTokens(combinedXacro, xacroArgs, options, absolutePath.parent_path());
            combinedXacro = ProcessXacroIncludes(combinedXacro, absolutePath.parent_path(), options, includeStack, xacroArgs);
            std::unordered_map<std::string, std::string> properties = xacroArgs;
            const std::regex propertyRegex(R"(<xacro:property\b([^>]*)/>)", std::regex::icase);
            for (auto it = std::sregex_iterator(combinedXacro.begin(), combinedXacro.end(), propertyRegex); it != std::sregex_iterator(); ++it)
            {
                const auto attributes = ParseAttributes((*it)[1].str());
                const auto nameIt = attributes.find("name");
                const auto valueIt = attributes.find("value");
                if (nameIt != attributes.end() && valueIt != attributes.end())
                {
                    properties[nameIt->second] = SubstituteXacroExpressions(valueIt->second, properties);
                }
            }
            const std::string synthesizedBodies = ExpandXacroMacroBodiesFallback(combinedXacro, properties);
            if (xml.find("<joint") == std::string::npos && !synthesizedBodies.empty())
            {
                xml += "\n" + synthesizedBodies;
            }
        }
        result.ExpandedRobotXml = xml;

        std::string robotName = options.RobotNameOverride;
        if (robotName.empty())
        {
            const std::regex robotRegex(R"(<robot\b([^>]*)>)", std::regex::icase);
            std::smatch robotMatch;
            if (std::regex_search(xml, robotMatch, robotRegex))
            {
                robotName = ExtractAttribute(robotMatch[1].str(), "name");
            }
        }
        if (robotName.empty())
        {
            robotName = absolutePath.stem().string();
        }

        result.Robot = apex::core::SerialRobotModel(robotName);
        result.Project.SchemaVersion = 4;
        result.Project.ProjectName = robotName + " imported project";
        result.Project.RobotName = robotName;
        result.Project.DescriptionSource.SourcePath = absolutePath.string();
        result.Project.DescriptionSource.SourceKind = result.UsedXacro ? "xacro" : "urdf";
        result.Project.DescriptionSource.PackageDependencies = CollectDescriptionPackageDependencies(
            result.UsedXacro ? sourceText : xml,
            absolutePath.parent_path());

        const std::regex jointRegex(R"(<joint\b([^>]*)>([\s\S]*?)</joint>)", std::regex::icase);
        for (auto iterator = std::sregex_iterator(xml.begin(), xml.end(), jointRegex); iterator != std::sregex_iterator(); ++iterator)
        {
            ++result.TotalJointElements;
            const std::string headerAttributes = (*iterator)[1].str();
            const std::string body = (*iterator)[2].str();
            const std::string jointType = ExtractAttribute(headerAttributes, "type");
            const std::string jointName = ExtractAttribute(headerAttributes, "name");

            if (jointType == "fixed")
            {
                result.Warnings.push_back("Skipped fixed joint '" + jointName + "'.");
                continue;
            }
            if (jointType != "revolute" && jointType != "continuous")
            {
                result.Warnings.push_back("Skipped unsupported joint type '" + jointType + "' for joint '" + jointName + "'.");
                continue;
            }

            std::vector<double> originValues;
            const std::regex originRegex(R"(<origin\b([^>]*)/?>)", std::regex::icase);
            std::smatch originMatch;
            if (std::regex_search(body, originMatch, originRegex))
            {
                originValues = ParseThreeValues(ExtractAttribute(originMatch[1].str(), "xyz"));
            }

            std::string limitAttributes;
            const std::regex limitRegex(R"(<limit\b([^>]*)/?>)", std::regex::icase);
            std::smatch limitMatch;
            if (std::regex_search(body, limitMatch, limitRegex))
            {
                limitAttributes = limitMatch[1].str();
            }

            const double minLimit = jointType == "continuous"
                ? -options.DefaultJointLimitRad
                : ParseOptionalDouble(ExtractAttribute(limitAttributes, "lower"), -options.DefaultJointLimitRad);
            const double maxLimit = jointType == "continuous"
                ? options.DefaultJointLimitRad
                : ParseOptionalDouble(ExtractAttribute(limitAttributes, "upper"), options.DefaultJointLimitRad);
            const double linkLength = ComputeLinkLengthFromOrigin(originValues, options.DefaultLinkLengthMeters, options.UseFullOriginNormForLinkLength);

            apex::core::JointDefinition joint;
            joint.Name = jointName.empty() ? ("J" + std::to_string(result.ImportedRevoluteJointCount + 1)) : jointName;
            joint.LinkLength = linkLength;
            joint.MinAngleRad = minLimit;
            joint.MaxAngleRad = maxLimit;
            result.Robot.AddRevoluteJoint(joint);
            result.Project.Joints.push_back(joint);
            ++result.ImportedRevoluteJointCount;
        }

        if (result.ImportedRevoluteJointCount == 0)
        {
            MaybeDumpXml(xml);
            throw std::invalid_argument("URDF/Xacro import found no supported revolute/continuous joints.");
        }

        if (options.CaptureMeshResources)
        {
            result.Project.MeshResources = ParseMeshResources(xml, absolutePath.parent_path(), options);
            result.ResolvedMeshResourceCount = std::count_if(result.Project.MeshResources.begin(), result.Project.MeshResources.end(), [](const apex::io::MeshResource& mesh)
            {
                return !mesh.ResolvedPath.empty();
            });
        }

        result.Project.Motion.Start.JointAnglesRad.assign(result.ImportedRevoluteJointCount, 0.0);
        result.Project.Motion.Goal.JointAnglesRad.assign(result.ImportedRevoluteJointCount, 0.0);
        if (result.ImportedRevoluteJointCount >= 1)
        {
            result.Project.Motion.Goal.JointAnglesRad[0] = apex::core::DegreesToRadians(20.0);
        }
        if (result.ImportedRevoluteJointCount >= 2)
        {
            result.Project.Motion.Goal.JointAnglesRad[1] = apex::core::DegreesToRadians(-15.0);
        }
        if (result.ImportedRevoluteJointCount >= 3)
        {
            result.Project.Motion.Goal.JointAnglesRad[2] = apex::core::DegreesToRadians(10.0);
        }
        result.Project.Motion.SampleCount = 20;
        result.Project.Motion.DurationSeconds = 5.0;
        result.Project.Motion.LinkSafetyRadius = 0.08;

        result.Project.Metadata.ProcessFamily = result.UsedXacro ? "robot_description/xacro" : "robot_description/urdf";
        result.Project.Metadata.Notes = result.Project.MeshResources.empty()
            ? "Imported robot description without mesh resources captured."
            : "Imported robot description with " + std::to_string(result.Project.MeshResources.size()) + " mesh resource references (" + std::to_string(result.ResolvedMeshResourceCount) + " resolved).";

        result.Project.Obstacles.push_back({ "ImportedFixture", {{0.55, 0.10, -0.20}, {0.85, 0.35, 0.20}} });
        result.Project.Obstacles.push_back({ "SafetyZone", {{0.95, -0.80, -0.50}, {1.10, 0.80, 0.50}} });
        return result;
    }
}
