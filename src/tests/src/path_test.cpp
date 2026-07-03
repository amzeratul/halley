#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

TEST(HalleyPath, Normalization)
{
	EXPECT_NE(Path(""), Path("."));
	EXPECT_EQ(Path("./"), Path("."));
	EXPECT_NE(Path("/."), Path("."));

	EXPECT_EQ(Path("foo"), Path("./foo"));
	EXPECT_EQ(Path("foo"), Path("./././foo"));

	EXPECT_EQ(Path("foo/bar"), Path("foo///bar"));

	EXPECT_NE(Path("foo"), Path("foo/."));
}

TEST(HalleyPath, Append)
{
	EXPECT_EQ(Path("foo") / Path("bar"), Path("foo/bar"));
	EXPECT_EQ(Path("foo/bar/baz") / Path("../.."), Path("foo/."));
	EXPECT_EQ(Path("foo") / Path("../.."), Path(".."));
}

TEST(HalleyPath, MakeRelative)
{
	const auto absFoo = Path("c:/foo");
	const auto absFooBar = Path("c:/foo/bar");
	const auto absFooBaz = Path("c:/foo/baz");
	
	EXPECT_EQ(absFooBar.makeRelativeTo(absFoo), Path("bar"));
	EXPECT_EQ(absFooBaz.makeRelativeTo(absFooBar), Path("../baz"));

	const auto relFoo = Path("foo");
	const auto relFooBar = Path("foo/bar");
	const auto relFooBaz = Path("foo/baz");

	EXPECT_EQ(relFooBar.makeRelativeTo(relFoo), Path("bar"));
	EXPECT_EQ(relFooBaz.makeRelativeTo(relFooBar), Path("../baz"));

	const auto dotRelFooBar = Path("./foo/bar");
	const auto dotRelFooBaz = Path("./foo/baz");

	EXPECT_EQ(dotRelFooBar.makeRelativeTo(relFoo), Path("bar"));
	EXPECT_EQ(dotRelFooBaz.makeRelativeTo(relFooBar), Path("../baz"));
}

TEST(HalleyPath, IsAbsolute)
{
	EXPECT_EQ(Path("c:/foo").isAbsolute(), true);
	EXPECT_EQ(Path("z:/foo/bar/").isAbsolute(), true);
	EXPECT_EQ(Path("/foo").isAbsolute(), true);
	EXPECT_EQ(Path("foo").isAbsolute(), false);
	EXPECT_EQ(Path("foo/bar").isAbsolute(), false);
}

TEST(HalleyPath, GetFilename)
{
	EXPECT_EQ(Path("/foo").getFilename(), "");
	EXPECT_EQ(Path("/foo/.").getFilename(), "");
	EXPECT_EQ(Path(".png").getFilename(), ".png");
	EXPECT_EQ(Path("bar.png").getFilename(), "bar.png");
	EXPECT_EQ(Path("/foo/bar.png").getFilename(), "bar.png");
	EXPECT_EQ(Path("/foo/.png").getFilename(), ".png");
	EXPECT_EQ(Path("/foo/.foo.png").getFilename(), ".foo.png");
	EXPECT_EQ(Path("/foo/.foo...png").getFilename(), ".foo...png");
}

TEST(HalleyPath, GetExtension)
{
	EXPECT_EQ(Path("/foo").getExtension(), "");
	EXPECT_EQ(Path("/foo/.").getExtension(), "");
	EXPECT_EQ(Path(".png").getExtension(), ".png");
	EXPECT_EQ(Path("bar.png").getExtension(), ".png");
	EXPECT_EQ(Path("/foo/bar.png").getExtension(), ".png");
	EXPECT_EQ(Path("/foo/.png").getExtension(), ".png");
	EXPECT_EQ(Path("/foo/.foo.png").getExtension(), ".png");
	EXPECT_EQ(Path("/foo/.foo...png").getExtension(), ".png");
}

TEST(HalleyPath, ReplaceExtension)
{
	EXPECT_EQ(Path(".png").replaceExtension(".txt"), ".txt");
	EXPECT_EQ(Path("bar.png").replaceExtension(".txt"), "bar.txt");
	EXPECT_EQ(Path("/foo/bar.png").replaceExtension(".txt"), "bar.txt");
	EXPECT_EQ(Path("/foo/.png").replaceExtension(".txt"), ".txt");
	EXPECT_EQ(Path("/foo/.foo.png").replaceExtension(".txt"), ".foo.txt");
	EXPECT_EQ(Path("/foo/.foo...png").replaceExtension(".txt"), ".foo...txt");
}
