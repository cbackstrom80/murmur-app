#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace pw8::plugin::content::detail
{
    struct MetadataFields
    {
        std::string name;
        std::string category;
        std::string description;
        std::string author;
        std::string enginesSummary;
        std::vector<std::string> moods;
        std::vector<std::string> genres;
        std::vector<std::string> tags;
    };

    struct FilterFields
    {
        std::string query;
        std::string category;
        std::string mood;
        std::string genre;
        std::string tag;
        bool favoritesOnly = false;
    };

    [[nodiscard]] inline std::string toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    [[nodiscard]] inline std::string trim(const std::string& value)
    {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            return {};
        const auto end = value.find_last_not_of(" \t\r\n");
        return value.substr(start, end - start + 1);
    }

    [[nodiscard]] inline bool arrayContainsIgnoreCase(const std::vector<std::string>& arr, const std::string& value)
    {
        const auto needle = toLower(trim(value));
        if (needle.empty())
            return false;
        for (const auto& item : arr)
        {
            if (toLower(trim(item)) == needle)
                return true;
        }
        return false;
    }

    [[nodiscard]] inline std::vector<std::string> tokenizeQuery(const std::string& query)
    {
        std::vector<std::string> tokens;
        std::string current;
        for (const char c : query)
        {
            if (std::isspace(static_cast<unsigned char>(c)) != 0)
            {
                const auto trimmed = trim(current);
                if (!trimmed.empty())
                    tokens.push_back(toLower(trimmed));
                current.clear();
                continue;
            }
            current += c;
        }

        const auto trimmed = trim(current);
        if (!trimmed.empty())
            tokens.push_back(toLower(trimmed));
        return tokens;
    }

    [[nodiscard]] inline std::string buildSearchHaystack(const MetadataFields& entry)
    {
        std::string haystack = toLower(entry.name + " " + entry.description + " " + entry.category + " "
                                         + entry.author + " " + entry.enginesSummary);
        for (const auto& mood : entry.moods)
            haystack += " " + toLower(mood);
        for (const auto& genre : entry.genres)
            haystack += " " + toLower(genre);
        for (const auto& tag : entry.tags)
            haystack += " " + toLower(tag);
        return haystack;
    }

    [[nodiscard]] inline bool isContextCrossoverMood(const std::string& mood) noexcept
    {
        static const char* kContextTokens[] = {"cinematic", "score", "trailer", "game",      "worship",
                                               "sleep",     "demo",  "ambient", "sound-design", nullptr};
        const auto lower = toLower(trim(mood));
        for (std::size_t i = 0; kContextTokens[i] != nullptr; ++i)
        {
            if (lower == kContextTokens[i])
                return true;
        }
        return false;
    }

    [[nodiscard]] inline bool entryMatchesFilter(const MetadataFields& entry, const FilterFields& filter,
                                                 bool isFavorite)
    {
        if (filter.favoritesOnly && !isFavorite)
            return false;

        const auto cat = toLower(trim(filter.category));
        if (!cat.empty() && toLower(trim(entry.category)) != cat)
            return false;

        const auto mood = toLower(trim(filter.mood));
        if (!mood.empty() && !arrayContainsIgnoreCase(entry.moods, mood))
            return false;

        const auto genre = toLower(trim(filter.genre));
        if (!genre.empty() && !arrayContainsIgnoreCase(entry.genres, genre)
            && !arrayContainsIgnoreCase(entry.moods, genre) && !arrayContainsIgnoreCase(entry.tags, genre))
            return false;

        const auto tag = toLower(trim(filter.tag));
        if (!tag.empty() && !arrayContainsIgnoreCase(entry.tags, tag)
            && !arrayContainsIgnoreCase(entry.genres, tag))
            return false;

        const auto tokens = tokenizeQuery(filter.query);
        if (!tokens.empty())
        {
            const std::string haystack = buildSearchHaystack(entry);
            for (const auto& token : tokens)
            {
                if (haystack.find(token) == std::string::npos)
                    return false;
            }
        }

        return true;
    }

    inline void collectGenreFacetValues(const MetadataFields& entry, std::vector<std::string>& out)
    {
        auto addUnique = [&](const std::string& value) {
            const auto trimmed = trim(value);
            if (trimmed.empty())
                return;
            if (std::find_if(out.begin(), out.end(), [&](const std::string& existing) {
                    return toLower(existing) == toLower(trimmed);
                }) == out.end())
                out.push_back(trimmed);
        };

        for (const auto& genre : entry.genres)
            addUnique(genre);
        for (const auto& mood : entry.moods)
        {
            if (isContextCrossoverMood(mood))
                addUnique(mood);
        }
        for (const auto& tag : entry.tags)
        {
            const auto lower = toLower(trim(tag));
            if (lower.find('-') != std::string::npos || lower == "interstellar" || lower.find("hoover") != std::string::npos)
                addUnique(tag);
        }
    }

} // namespace pw8::plugin::content::detail
