#include "content/PresetMetadataFilterLogic.hpp"

#include <catch2/catch_test_macros.hpp>

using pw8::plugin::content::detail::FilterFields;
using pw8::plugin::content::detail::MetadataFields;
using pw8::plugin::content::detail::arrayContainsIgnoreCase;
using pw8::plugin::content::detail::buildSearchHaystack;
using pw8::plugin::content::detail::collectGenreFacetValues;
using pw8::plugin::content::detail::entryMatchesFilter;
using pw8::plugin::content::detail::isContextCrossoverMood;
using pw8::plugin::content::detail::tokenizeQuery;

TEST_CASE("Preset metadata filter matches category and mood", "[preset-index]")
{
    MetadataFields entry;
    entry.category = "bass";
    entry.moods = {"dark", "hoover"};
    entry.genres = {"hoover-bass"};
    entry.tags = {"club", "hoover-bass"};

    FilterFields filter;
    filter.category = "bass";
    REQUIRE(entryMatchesFilter(entry, filter, false));

    filter.category = {};
    filter.mood = "dark";
    REQUIRE(entryMatchesFilter(entry, filter, false));

    filter.mood = "bright";
    REQUIRE_FALSE(entryMatchesFilter(entry, filter, false));
}

TEST_CASE("Preset metadata filter genre crossover matches tags and moods", "[preset-index]")
{
    MetadataFields entry;
    entry.category = "pad";
    entry.moods = {"cinematic"};
    entry.tags = {"interstellar"};

    FilterFields filter;
    filter.genre = "cinematic";
    REQUIRE(entryMatchesFilter(entry, filter, false));

    filter.genre = "interstellar";
    REQUIRE(entryMatchesFilter(entry, filter, false));

    filter.genre = "hoover-bass";
    entry.genres = {"hoover-bass"};
    REQUIRE(entryMatchesFilter(entry, filter, false));
}

TEST_CASE("Preset metadata filter favorites gate", "[preset-index]")
{
    MetadataFields entry;
    entry.category = "lead";
    entry.name = "TEST LEAD";

    FilterFields filter;
    filter.favoritesOnly = true;
    REQUIRE_FALSE(entryMatchesFilter(entry, filter, false));
    REQUIRE(entryMatchesFilter(entry, filter, true));
}

TEST_CASE("Preset metadata filter query tokenizes as AND across fields", "[preset-index]")
{
    MetadataFields entry;
    entry.name = "Interstellar Drift";
    entry.category = "pad";
    entry.author = "MURMUR";
    entry.moods = {"cinematic"};
    entry.tags = {"interstellar"};

    FilterFields filter;
    filter.query = "interstellar pad";
    REQUIRE(entryMatchesFilter(entry, filter, false));

    filter.query = "interstellar bass";
    REQUIRE_FALSE(entryMatchesFilter(entry, filter, false));

    filter.query = "murmur cinematic";
    REQUIRE(entryMatchesFilter(entry, filter, false));
}

TEST_CASE("Preset metadata filter query tokenization trims whitespace", "[preset-index]")
{
    MetadataFields entry;
    entry.name = "Hoover Pulse";
    entry.tags = {"hoover-bass"};

    FilterFields filter;
    filter.query = "  hoover   pulse  ";
    REQUIRE(entryMatchesFilter(entry, filter, false));
}

TEST_CASE("Search haystack includes author and engines summary", "[preset-index]")
{
    MetadataFields entry;
    entry.name = "Init";
    entry.author = "Ben Bethurum";
    entry.enginesSummary = "01, 03";

    FilterFields filter;
    filter.query = "bethurum";
    REQUIRE(entryMatchesFilter(entry, filter, false));

    filter.query = "03";
    REQUIRE(entryMatchesFilter(entry, filter, false));
}

TEST_CASE("Genre facet collection includes crossover tags", "[preset-index]")
{
    MetadataFields entry;
    entry.genres = {"score"};
    entry.moods = {"cinematic"};
    entry.tags = {"interstellar", "hoover-bass"};

    std::vector<std::string> values;
    collectGenreFacetValues(entry, values);

    REQUIRE(arrayContainsIgnoreCase(values, "score"));
    REQUIRE(arrayContainsIgnoreCase(values, "cinematic"));
    REQUIRE(arrayContainsIgnoreCase(values, "interstellar"));
    REQUIRE(arrayContainsIgnoreCase(values, "hoover-bass"));
    REQUIRE(isContextCrossoverMood("cinematic"));
}
