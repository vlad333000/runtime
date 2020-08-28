#include "default.h"
#include "../runtime/util.h"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <iterator>

#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
#include <iostream>
#endif // DF__SQF_FILEIO__TRACE_REESOLVE

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

template<char delimiter>
class StringDelimiter : public std::string
{ };
template<char delimiter>
std::istream & operator>>(std::istream & is, StringDelimiter<delimiter>& output)
{
    std::getline(is, output, delimiter);
    return is;
}

inline bool file_exists(std::filesystem::path p)
{
    std::ifstream infile(p.string());
    return infile.good();
}

std::optional<sqf::runtime::fileio::pathinfo> sqf::fileio::impl_default::get_info_virtual(std::string_view viewVirtual, sqf::runtime::fileio::pathinfo current) const
{
    // Create & Cleanse stuff
    auto virt = std::string(viewVirtual);
    std::replace(virt.begin(), virt.end(), '\\', '/');
    virt = std::string(sqf::runtime::util::trim(virt));
    std::string virtFull = virt;

#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
        "        " <<
        "        " <<
        "    " << "\x1B[36mget_info_virtual\033[0m(\x1B[90m" << virt << "\033[0m, \x1B[90m{" << current.physical << ", " << current.virtual_ << "}\033[0m) requested" << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE

    // Abort conditions
    if (virt.empty())
    {
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
        std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
            "        " <<
            "        " <<
            "    " << "\x1B[36mget_info_virtual\033[0m" <<
            "    " << "provided virtual path is empty. \x1B[31mReturning FileNotFound\033[0m." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
        return {};
    }

    // Prepare local tree-node list
    std::vector<std::shared_ptr<path_element>> nodes;
    nodes.push_back(m_virtual_file_root);

#if WIN32
    if (virt[0] != '/' && !(virt.length() >= 2 && virt[1] == ':'))
#else
    if (virt[0] != '/')
#endif
    { // Navigate current virtual path if relative

        // Iterate over the whole existing virtual path
        if (!current.virtual_.empty())
        {
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
            std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                "        " <<
                "        " <<
                "    " << "\x1B[36mget_info_virtual\033[0m" <<
                "    " << "Navigating Relative-Path provided:" << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE

            virtFull = current.virtual_ + "/" + virt;

            std::istringstream stream_virt(current.virtual_);
            for (auto it = std::istream_iterator<StringDelimiter<'/'>>{ stream_virt }; it != std::istream_iterator<StringDelimiter<'/'>>{}; ++it)
            {
                if (it->empty())
                { /* skip empty */
                    continue;
                }
                if (nodes.back()->next.find(*it) != nodes.back()->next.end())
                {
                    nodes.push_back(nodes.back()->next.at(*it));
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
                    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                        "        " <<
                        "        " <<
                        "    " << "\x1B[36mget_info_virtual\033[0m" <<
                        "    " << "    " << "Navigated '" << *it << "'." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
                }
                else
                { /* Dead-End. File Not Found. */
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
                    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                        "        " <<
                        "        " <<
                        "    " << "\x1B[36mget_info_virtual\033[0m" <<
                        "    " << "    " << "Reached dead end with '" << *it << "'. \x1B[31mReturning FileNotFound\033[0m." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
                    return {};
                }
            }
        }
    }

    // Explore further until we hit dead-end
    {
        std::istringstream stream_virt(virt);
        auto it = std::istream_iterator<StringDelimiter<'/'>>{ stream_virt };
        for (; it != std::istream_iterator<StringDelimiter<'/'>>{}; ++it)
        {
            if (it->empty()) { /* skip empty */ continue; }
            if (*it == ".."s && !nodes.empty())
            {
                // Move dir-up
                nodes.pop_back();
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
                std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                    "        " <<
                    "        " <<
                    "    " << "\x1B[36mget_info_virtual\033[0m" <<
                    "    " << "    " << "Moved dir up." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
            }
            else
            {
                if (nodes.empty())
                {
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
                    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                        "        " <<
                        "        " <<
                        "    " << "\x1B[36mget_info_virtual\033[0m" <<
                        "    " << "    " << "Nodes are empty. Aborting." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
                    break;
                }
                else if (nodes.back()->next.find(*it) == nodes.back()->next.end())
                { /* Dead-End.  */
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
                    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                        "        " <<
                        "        " <<
                        "    " << "\x1B[36mget_info_virtual\033[0m" <<
                        "    " << "    " << "Reached dead end while exploring path on '" << *it << "'." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
                    break;
                }
                else
                {
                    nodes.push_back(nodes.back()->next.at(*it));
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
                    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                        "        " <<
                        "        " <<
                        "    " << "\x1B[36mget_info_virtual\033[0m" <<
                        "    " << "    " << "Navigated '" << *it << "'." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
                }
            }
        }

        
        if (nodes.empty())
        { /* Invalid path from our perspective. Return File-Not-Found. */

#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
            std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                "        " <<
                "        " <<
                "    " << "\x1B[36mget_info_virtual\033[0m" <<
                "    " << "    " << "No navigation nodes found. \x1B[31mReturning FileNotFound\033[0m." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
            return {};
        }

        // Set virtual to remaining and ensure no further dir-up occur
        virt.clear();
        for (; it != std::istream_iterator<StringDelimiter<'/'>>{}; ++it)
        {
            if (*it == ".."s) { /* skip dir-up */ continue; }
            virt.append("/");
            virt.append(*it);
        }
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
        std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
            "        " <<
            "        " <<
            "    " << "\x1B[36mget_info_virtual\033[0m" <<
            "    " << "    " << "Built remainder '" << virt << "'." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
    }
    // Check every physical path in current tree_element if the file exists
    for (auto& phys : nodes.back()->physical)
    {
        auto tmp = phys.string() + virt;
        std::filesystem::path p(tmp);
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
        std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
            "        " <<
            "        " <<
            "    " << "\x1B[36mget_info_virtual\033[0m" <<
            "    " << "    " << "Testing '" << p.string() << "'" << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
        if (file_exists(p))
        {
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
            std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                "        " <<
                "        " <<
                "    " << "\x1B[36mget_info_virtual\033[0m" <<
                "    " << "    " << "Match!. \x1B[32mReturning " << p.string() << "\033[0m." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
            return { { p.string(), virtFull } };
        }
    }

#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
        "        " <<
        "        " <<
        "    " << "\x1B[36mget_info_virtual\033[0m" <<
        "    " << "    " << "No matching file found. \x1B[31mReturning FileNotFound\033[0m." << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
    // As we reached this, file-not-found
    return {};
}
std::optional<sqf::runtime::fileio::pathinfo> sqf::fileio::impl_default::get_info_physical(std::string_view viewVirtual, sqf::runtime::fileio::pathinfo current) const
{
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
        "        " <<
        "        " <<
        "    " << "\x1B[36mget_info_physical\033[0m" <<
        "    " << "    " << "Attempting to find physical path for '" << viewVirtual << "'" << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE

    std::filesystem::path toFindPath(viewVirtual);
    toFindPath = toFindPath.lexically_normal();
    if (toFindPath.is_relative() || viewVirtual.size() > 3 && (viewVirtual.substr(0, 3) == "../"sv || viewVirtual.substr(0, 3) == "..\\"sv))
    {
        if (std::filesystem::is_regular_file(current.physical))
        {
            auto tmp = std::filesystem::path(current.physical);
            auto tmp2 = tmp.parent_path();
            toFindPath = tmp2 / toFindPath;
        }
        else
        {
            toFindPath = current.physical / toFindPath;
        }
        toFindPath = toFindPath.lexically_normal();
    }
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
    std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
        "        " <<
        "        " <<
        "    " << "\x1B[36mget_info_physical\033[0m" <<
        "    " << "    " << "Adjusted path to '" << toFindPath.string() << "'" << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
    for (auto& it : m_path_elements)
    {
        for (auto& phys : it->physical)
        {
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
            std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                "        " <<
                "        " <<
                "    " << "\x1B[36mget_info_physical\033[0m" <<
                "    " << "    " << "Comparing against '" << phys << "'" << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
            auto pair = std::mismatch(phys.begin(), phys.end(), toFindPath.begin());
            auto rootEnd = std::get<0>(pair);
            auto nothing = std::get<0>(pair);
            if (rootEnd == phys.end() && !std::equal(phys.begin(), phys.end(), toFindPath.begin(), toFindPath.end()))
            {
#ifdef DF__SQF_FILEIO__TRACE_REESOLVE
                std::cout << "\x1B[33m[FILEIO ASSERT]\033[0m" <<
                    "        " <<
                    "        " <<
                    "    " << "\x1B[36mget_info_physical\033[0m" <<
                    "    " << "    " << "Match! Passing to get_info_virtual" << std::endl;
#endif // DF__SQF_FILEIO__TRACE_REESOLVE
                auto toFindString = toFindPath.string();
                auto res = get_info_virtual(it->virtual_full + toFindString.substr(phys.string().size() + 1), current);
                if (res.has_value())
                {
                    return res;
                }
            }
        }
    }
    return {};
}

void sqf::fileio::impl_default::add_mapping(std::string_view viewPhysical, std::string_view viewVirtual)
{
    // Create & Cleanse stuff
    auto phys = std::string(viewPhysical);
    std::replace(phys.begin(), phys.end(), '\\', '/');
    auto path_phys = std::filesystem::path(phys);

    auto virt = std::string(viewVirtual);
    std::replace(virt.begin(), virt.end(), '\\', '/');

    // Iterate over the whole virtual path and add missing elements to the file_tree
    std::istringstream stream_virt(virt);
    std::shared_ptr<path_element> tree = m_virtual_file_root;
    std::vector<std::string> path_elements;
    for (auto it = std::istream_iterator<StringDelimiter<'/'>>{ stream_virt }; it != std::istream_iterator<StringDelimiter<'/'>>{}; ++it)
    {
        if (it->empty()) { /* skip empty */ continue; }
        path_elements.push_back(*it);
        auto res = tree->next.find(*it);
        if (res == tree->next.end())
        {
            std::stringstream sstream;
            for (auto& el : path_elements)
            {
                sstream << "/" << el;
            }
            tree = tree->next[*it] = std::make_shared<path_element>();
            tree->virtual_full = sstream.str();
            m_path_elements.push_back(tree);
        }
        else
        {
            tree = res->second;
        }
    }

    // Add physical path to final tree node
    tree->physical.push_back(std::filesystem::path(phys).lexically_normal());
}

std::string sqf::fileio::impl_default::read_file(sqf::runtime::fileio::pathinfo info) const
{
    auto res = sqf::runtime::fileio::read_file_from_disk(info.physical);
    return *res;
}
