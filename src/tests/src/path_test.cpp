#include <gtest/gtest.h>
#include <halley.hpp>
using namespace Halley;

TEST(HalleyPath, Identity)
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
	EXPECT_EQ(Path("foo/bar/baz") / Path("../.."), Path("foo"));
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
