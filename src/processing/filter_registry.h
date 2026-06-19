#pragma once

#include "filter_base.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class FilterRegistry
{
public:
    using Factory = std::function<std::unique_ptr<FilterBase>()>;

    static FilterRegistry& instance();

    void registerFilter(const std::string& key, Factory factory);
    std::unique_ptr<FilterBase> create(const std::string& key) const;

    // All registered keys
    std::vector<std::string> keys() const;

    // Keys grouped by category
    std::vector<std::string> categories() const;
    std::vector<std::string> keysInCategory(const std::string& category) const;

    // Get human-readable name for a key
    std::string name(const std::string& key) const;

private:
    FilterRegistry() = default;

    struct Entry
    {
        Factory factory;
        std::unique_ptr<FilterBase> prototype; // for metadata
    };
    std::unordered_map<std::string, Entry> m_entries;
};
