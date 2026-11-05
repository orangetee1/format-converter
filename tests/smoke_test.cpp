#include <gtest/gtest.h>

#include "core/version.hpp"

static_assert(__cplusplus >= 202002L, "lab4 must be compiled as C++20");

TEST(Smoke, VersionIsNotEmpty) { EXPECT_FALSE(lab4::version().empty()); }

TEST(Smoke, GoogleTestWorks) { EXPECT_EQ(2 + 2, 4) << "arithmetic is broken"; }
