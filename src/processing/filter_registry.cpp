#include "filter_registry.h"

#include <algorithm>
#include <set>

FilterRegistry& FilterRegistry::instance()
{
    static FilterRegistry reg;
    return reg;
}

void FilterRegistry::registerFilter(const std::string& key, Factory factory)
{
    Entry entry;
    entry.factory = std::move(factory);
    entry.prototype = entry.factory();
    m_entries[key] = std::move(entry);
}

std::unique_ptr<FilterBase> FilterRegistry::create(const std::string& key) const
{
    auto it = m_entries.find(key);
    if (it != m_entries.end())
        return it->second.factory();
    return nullptr;
}

std::vector<std::string> FilterRegistry::keys() const
{
    std::vector<std::string> result;
    for (const auto& pair : m_entries)
        result.push_back(pair.first);
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> FilterRegistry::categories() const
{
    std::set<std::string> cats;
    for (const auto& pair : m_entries)
        cats.insert(pair.second.prototype->category());
    return std::vector<std::string>(cats.begin(), cats.end());
}

std::vector<std::string> FilterRegistry::keysInCategory(const std::string& category) const
{
    std::vector<std::string> result;
    for (const auto& pair : m_entries)
    {
        if (pair.second.prototype->category() == category)
            result.push_back(pair.first);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::string FilterRegistry::name(const std::string& key) const
{
    auto it = m_entries.find(key);
    if (it != m_entries.end())
        return it->second.prototype->name();
    return key;
}
