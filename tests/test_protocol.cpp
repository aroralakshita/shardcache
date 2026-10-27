#include <gtest/gtest.h>

#include "protocol/protocol.h"

using namespace shardcache;
using namespace std::chrono_literals;

//valid commands
TEST(ParserTest, GetWithKey) {
    ParseResult r = parse_command("GET a");
    ASSERT_TRUE(r.command.has_value());
    EXPECT_EQ(r.command->type, CommandType::Get);
    EXPECT_EQ(r.command->key, "a");
}

TEST(ParserTest, SetWithoutTtl) {
    ParseResult r = parse_command("SET hello world");
    ASSERT_TRUE(r.command.has_value());
    EXPECT_EQ(r.command->type, CommandType::Set);
    EXPECT_EQ(r.command->key, "hello");
    EXPECT_EQ(r.command->value, "world");
    EXPECT_FALSE(r.command->ttl.has_value());
}

TEST(ParserTest, SetWithTtl) {
    ParseResult r = parse_command("SET a 1 30");
    ASSERT_TRUE(r.command.has_value());
    EXPECT_EQ(r.command->ttl, 30s);
}

TEST(ParserTest, DotValueWithoutTtlIsJustAValue) {
    ParseResult r = parse_command("SET hello .1");
    ASSERT_TRUE(r.command.has_value());
    EXPECT_EQ(r.command->value, ".1");
    EXPECT_FALSE(r.command->ttl.has_value());
}

TEST(ParserTest, CommandNameIsCaseInsensitive) {
    ParseResult r = parse_command("get a");
    ASSERT_TRUE(r.command.has_value());
    EXPECT_EQ(r.command->type, CommandType::Get);
}

TEST(ParserTest, ExtraSpacesAreIgnored) {
    ParseResult r = parse_command("  SET   a   1  ");
    ASSERT_TRUE(r.command.has_value());
    EXPECT_EQ(r.command->key, "a");
    EXPECT_EQ(r.command->value, "1");
}

TEST(ParserTest, KeysAreCaseSensitive) {
    ParseResult r = parse_command("GET Alice");
    ASSERT_TRUE(r.command.has_value());
    EXPECT_EQ(r.command->key, "Alice");
}

//invalid commands
void expect_error(const char* line) {
    ParseResult r = parse_command(line);
    EXPECT_FALSE(r.command.has_value()) << "input: \"" << line << "\"";
    EXPECT_FALSE(r.error.empty()) << "input: \"" << line << "\"";
}

TEST(ParserTest, EmptyOrBlankLine) {
    expect_error("");
    expect_error("     ");
}

TEST(ParserTest, UnknownCommand) {
    expect_error("FLY a");
}

TEST(ParserTest, WrongNumberOfWords) {
    expect_error("GET");
    expect_error("GET a b");
    expect_error("DEL");
    expect_error("SET a");
    expect_error("SET a 1 30 extra");
}

TEST(ParserTest, ValueWithSpaceIsMisreadAsTtl) {
    expect_error("SET greeting hello world");
}

TEST(ParserTest, BadTtlValues) {
    expect_error("SET a hello .1");  
    expect_error("SET a 1 0");
    expect_error("SET a 1 -5");
    expect_error("SET a 1 10x");    
    expect_error("SET a 1 abc");
    expect_error("SET a 1 999999999999");
}
