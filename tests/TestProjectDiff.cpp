#include "TestFramework.h"

#include "apex/diff/ProjectDiff.h"

ARS_TEST(TestProjectDiffDetectsObstacleAndMetadataChanges)
{
    apex::io::StudioProject lhs;
    lhs.ProjectName = "A";
    lhs.Metadata.Owner = "one";
    lhs.Obstacles.push_back({"Fixture", {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}}});

    apex::io::StudioProject rhs = lhs;
    rhs.Metadata.Owner = "two";
    rhs.Obstacles[0].Bounds.Max.X = 2.0;

    const apex::diff::ProjectDiffEngine engine;
    const auto entries = engine.Compare(lhs, rhs);

    apex::tests::Require(entries.size() >= 2, "Diff must detect metadata and obstacle changes.");
}
