#pragma once
#include "../fileio.h"
#include "../diagnostics/diag_info.h"

#include <string>
#include <string_view>
#include <vector>


namespace sqf
{
    namespace runtime
    {
        class runtime;
        namespace parser
        {
            class macro 
            {
            public:
                // Special method pointer that may be filled
                // to give this macro a special behavior rather
                // then a content.
                // Gets only applied if pointer is != nullptr
                // m -> the original macro
                // local_fileinfo -> the location, where the macro is called locally (lowest level)
                // original_fileinfo -> the most upper file in the macro chain
                // params -> the passed parameters
                typedef std::string(*callback)(
                    const macro& m,
                    const ::sqf::runtime::diagnostics::diag_info dinf,
                    const ::sqf::runtime::fileio::pathinfo location,
                    const std::vector<std::string>& params,
                    ::sqf::runtime::runtime& runtime
                    );
            private:
                std::string m_name;
                std::string m_content;
                std::vector<std::string> m_args;
                bool m_is_callable;
                callback m_callback;

                ::sqf::runtime::diagnostics::diag_info m_diag_info;
            public:
                macro() = default;
                macro(diagnostics::diag_info diag_info, std::string name, std::vector<std::string> args, bool is_callable, std::string content, callback callback) :
                    m_name(name),
                    m_content(content),
                    m_args(args),
                    m_is_callable(is_callable),
                    m_callback(callback),
                    m_diag_info(diag_info) {}

                macro(diagnostics::diag_info diag_info, std::string name) : macro(diag_info, name, {}, false, {}, nullptr) {}
                macro(diagnostics::diag_info diag_info, std::string name, std::string content) : macro(diag_info, name, {}, false, content, nullptr) {}
                macro(diagnostics::diag_info diag_info, std::string name, std::vector<std::string> args) : macro(diag_info, name, args, true, {}, nullptr) {}
                macro(diagnostics::diag_info diag_info, std::string name, std::vector<std::string> args, std::string content) : macro(diag_info, name, args, true, content, nullptr) {}
                macro(std::string name) : macro({}, name, {}, false, {}, nullptr) {}
                macro(std::string name, std::string content) : macro({}, name, {}, false, content, nullptr) {}
                macro(std::string name, callback callback) : macro({}, name, {}, false, {}, callback) {}
                macro(std::string name, std::vector<std::string> args, std::string content) : macro({}, name, args, true, content, nullptr) {}
                macro(std::string name, std::vector<std::string> args, callback callback) : macro({}, name, args, true, {}, callback) {}

                std::string_view name() const { return m_name; }
                std::string_view content() const { return m_content; }
                const std::vector<std::string>& args() const { return m_args; }
                const ::sqf::runtime::diagnostics::diag_info diag_info() const { return m_diag_info; }
                bool has_callback() const { return m_callback; }
                bool is_callable() const { return m_is_callable; }
                
                std::string operator()(
                    const ::sqf::runtime::diagnostics::diag_info dinf,
                    const ::sqf::runtime::fileio::pathinfo location,
                    const std::vector<std::string>& params,
                    ::sqf::runtime::runtime& runtime) const { return m_callback(*this, dinf, location, params, runtime); }
            };
            class preprocessor
            {
            public:
                virtual void push_back(::sqf::runtime::parser::macro m) = 0;
                virtual ~preprocessor() {}
                virtual std::optional<std::string> preprocess(::sqf::runtime::runtime& runtime, ::std::string_view view, ::sqf::runtime::fileio::pathinfo pathinfo) = 0;
                std::optional<std::string> preprocess(::sqf::runtime::runtime& runtime, ::sqf::runtime::fileio::pathinfo pathinfo);
            };
        }
    }
    namespace parser::preprocessor
    {
        class passthrough : public ::sqf::runtime::parser::preprocessor
        {
            public:
                virtual void push_back(::sqf::runtime::parser::macro m) override {};
                virtual ~passthrough() override { return; };
                virtual std::optional<std::string> preprocess(::sqf::runtime::runtime& runtime, ::std::string_view view, ::sqf::runtime::fileio::pathinfo pathinfo) override;
        };
    }
}
