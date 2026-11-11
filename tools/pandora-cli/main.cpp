// pandora-cli — the command line front end.
//
// It works on files and directories, so it runs the same way on a laptop and on
// the console. Exporting a release is the job that matters most:
//
//     pandora-cli package pkg/ dist/PPSA77010.zip
//
// produces the archive the catalog expects, rooted at the title folder.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "pandora/json.hpp"
#include "pandora/log.hpp"
#include "pandora/param.hpp"
#include "pandora/platform.hpp"
#include "pandora/sha256.hpp"
#include "pandora/version.hpp"
#include "pandora/zip.hpp"

namespace {

constexpr int kOk = 0;
constexpr int kFailed = 1;
constexpr int kUsage = 2;

#ifdef PANDORA_VERSION
constexpr const char* kVersion = PANDORA_VERSION;
#else
constexpr const char* kVersion = "0.0.0-dev";
#endif

void print_usage() {
    std::printf(
        "pandora-cli "
        "%s"
        "\n"
        "\n"
        "Usage: pandora-cli <command> [arguments]\n"
        "\n"
        "Commands:\n"
        "  title-id check <TITLEID>        validate a title id\n"
        "  param show <file>               print the fields of a param.json\n"
        "  param validate <file>           check a param.json\n"
        "  param bump <file> [--minor|--major]\n"
        "                                  raise contentVersion in place (patch by default)\n"
        "  digest <file>                   print the SHA-256 of a file\n"
        "  package <dir> <out.zip>         build a release archive rooted at <TITLEID>/\n"
        "  doctor                          run self-checks\n"
        "  version                         print the version\n"
        "  help                            print this message\n",
        kVersion);
}

int usage_error(const std::string& message) {
    std::fprintf(stderr, "error: %s\n\n", message.c_str());
    print_usage();
    return kUsage;
}

bool read_text(const std::string& path, std::string* out, std::string* error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        *error = "cannot open " + path;
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    *out = buffer.str();
    return true;
}

int command_title_id_check(const std::string& id) {
    std::string error;
    if (!pandora::validate_title_id(id, &error)) {
        std::printf("%s: invalid — %s\n", id.c_str(), error.c_str());
        return kFailed;
    }
    std::printf("%s: valid\n", id.c_str());
    return kOk;
}

int command_param_show(const std::string& path) {
    std::string text;
    std::string error;
    if (!read_text(path, &text, &error)) {
        std::fprintf(stderr, "error: %s\n", error.c_str());
        return kFailed;
    }

    const auto param = pandora::ParamJson::parse(text, &error);
    if (!param.has_value()) {
        std::fprintf(stderr, "error: %s is not valid param.json: %s\n", path.c_str(),
                     error.c_str());
        return kFailed;
    }

    std::printf("title id        : %s\n", param->title_id.c_str());
    std::printf("content version : %s\n", param->content_version.c_str());
    if (param->master_version.has_value()) {
        std::printf("master version  : %s\n", param->master_version->c_str());
    } else {
        std::printf("master version  : (absent)\n");
    }
    return kOk;
}

int command_param_validate(const std::string& path) {
    std::string text;
    std::string error;
    if (!read_text(path, &text, &error)) {
        std::fprintf(stderr, "error: %s\n", error.c_str());
        return kFailed;
    }

    const auto param = pandora::ParamJson::parse(text, &error);
    if (!param.has_value()) {
        std::printf("%s: invalid — %s\n", path.c_str(), error.c_str());
        return kFailed;
    }
    if (!param->validate(&error)) {
        std::printf("%s: invalid — %s\n", path.c_str(), error.c_str());
        return kFailed;
    }
    std::printf("%s: valid\n", path.c_str());
    return kOk;
}

int command_param_bump(const std::string& path, const std::string& field) {
    std::string text;
    std::string error;
    if (!read_text(path, &text, &error)) {
        std::fprintf(stderr, "error: %s\n", error.c_str());
        return kFailed;
    }

    // Edit the document rather than a fixed field list, so anything else the
    // file carries survives the round trip.
    const auto document = pandora::json::parse_flat_object(text, &error);
    if (!document.has_value()) {
        std::fprintf(stderr, "error: %s is not a flat JSON object: %s\n", path.c_str(),
                     error.c_str());
        return kFailed;
    }

    const auto current = document->get("contentVersion");
    if (!current.has_value()) {
        std::fprintf(stderr, "error: contentVersion is missing from %s\n", path.c_str());
        return kFailed;
    }

    const auto parsed = pandora::ContentVersion::parse(*current);
    if (!parsed.has_value()) {
        std::fprintf(stderr, "error: contentVersion '%s' is not NN.NNN.NNN\n", current->c_str());
        return kFailed;
    }

    pandora::ContentVersion raised = *parsed;
    if (field == "--minor") {
        raised = parsed->bump_minor();
    } else if (field == "--major") {
        raised = parsed->bump_major();
    } else {
        raised = parsed->bump_patch();
    }

    pandora::json::FlatObject edited = *document;
    edited.replace("contentVersion", raised.str());

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        std::fprintf(stderr, "error: cannot write %s\n", path.c_str());
        return kFailed;
    }
    output << pandora::json::to_pretty_string(edited);
    if (!output) {
        std::fprintf(stderr, "error: write failed for %s\n", path.c_str());
        return kFailed;
    }

    std::printf("%s: %s -> %s\n", path.c_str(), parsed->str().c_str(), raised.str().c_str());
    return kOk;
}

int command_digest(const std::string& path) {
    std::string error;
    const auto digest = pandora::sha256_file(path, &error);
    if (!digest.has_value()) {
        std::fprintf(stderr, "error: %s\n", error.c_str());
        return kFailed;
    }
    std::printf("%s  %s\n", digest->c_str(), path.c_str());
    return kOk;
}

int command_package(const std::string& directory, const std::string& output_path) {
    namespace fs = std::filesystem;

    const fs::path root(directory);
    std::error_code fs_error;
    if (!fs::is_directory(root, fs_error)) {
        std::fprintf(stderr, "error: %s is not a directory\n", directory.c_str());
        return kFailed;
    }

    const fs::path param_path = root / "sce_sys" / "param.json";
    if (!fs::is_regular_file(param_path, fs_error)) {
        std::fprintf(stderr,
                     "error: %s does not exist; a title package needs sce_sys/param.json at its "
                     "root\n",
                     param_path.string().c_str());
        return kFailed;
    }

    std::string text;
    std::string error;
    if (!read_text(param_path.string(), &text, &error)) {
        std::fprintf(stderr, "error: %s\n", error.c_str());
        return kFailed;
    }

    const auto param = pandora::ParamJson::parse(text, &error);
    if (!param.has_value() || !param->validate(&error)) {
        std::fprintf(stderr, "error: %s is not valid: %s\n", param_path.string().c_str(),
                     error.c_str());
        return kFailed;
    }

    const std::string& title_id = param->title_id;

    std::vector<pandora::zip::Entry> entries;
    entries.push_back(pandora::zip::Entry{title_id + "/", ""});

    for (const auto& item : fs::recursive_directory_iterator(root)) {
        const std::string relative = fs::relative(item.path(), root, fs_error).generic_string();
        if (fs_error || relative.empty()) {
            continue;
        }

        if (item.is_directory(fs_error)) {
            entries.push_back(pandora::zip::Entry{title_id + "/" + relative + "/", ""});
            continue;
        }
        if (!item.is_regular_file(fs_error)) {
            continue;
        }

        std::string data;
        if (!read_text(item.path().string(), &data, &error)) {
            std::fprintf(stderr, "error: %s\n", error.c_str());
            return kFailed;
        }
        entries.push_back(pandora::zip::Entry{title_id + "/" + relative, std::move(data)});
    }

    // Deterministic order keeps the artifact hash stable across machines.
    std::sort(entries.begin(), entries.end(),
              [](const pandora::zip::Entry& a, const pandora::zip::Entry& b) {
                  const bool dir_a = !a.name.empty() && a.name.back() == '/';
                  const bool dir_b = !b.name.empty() && b.name.back() == '/';
                  if (dir_a != dir_b) {
                      return dir_a;
                  }
                  return a.name < b.name;
              });

    std::error_code create_error;
    const fs::path output(output_path);
    if (output.has_parent_path()) {
        fs::create_directories(output.parent_path(), create_error);
    }

    if (!pandora::zip::write_archive(output_path, entries, &error)) {
        std::fprintf(stderr, "error: %s\n", error.c_str());
        return kFailed;
    }

    std::printf("wrote %s\n", output_path.c_str());
    std::printf("  title id : %s\n", title_id.c_str());
    std::printf("  version  : %s\n", param->content_version.c_str());
    std::printf("  entries  : %zu\n", entries.size());
    for (const auto& entry : entries) {
        std::printf("    %s\n", entry.name.c_str());
    }

    const auto digest = pandora::sha256_file(output_path, &error);
    if (digest.has_value()) {
        std::printf("  sha256   : %s\n", digest->c_str());
    }
    return kOk;
}

int command_doctor() {
    int failures = 0;

    auto report = [&failures](bool passed, const std::string& name, const std::string& detail) {
        std::printf("[%s] %s%s\n", passed ? "PASS" : "FAIL", name.c_str(),
                    detail.empty() ? "" : (" — " + detail).c_str());
        if (!passed) {
            ++failures;
        }
    };

    // SHA-256 against the published vector for "abc".
    const std::string abc_digest = pandora::sha256_hex("abc");
    report(abc_digest == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
           "sha256 test vector", abc_digest);

    // CRC-32 against the published vector for "123456789".
    char crc_text[16];
    std::snprintf(crc_text, sizeof(crc_text), "%08x", pandora::zip::crc32("123456789"));
    report(std::string(crc_text) == "cbf43926", "crc32 test vector", crc_text);

    // Version parsing and the field-width clamp.
    const auto parsed = pandora::ContentVersion::parse("01.000.999");
    report(parsed.has_value() && parsed->bump_patch().str() == "01.000.999",
           "contentVersion clamp at 999", parsed.has_value() ? parsed->bump_patch().str() : "-");
    report(!pandora::ContentVersion::parse("1.0.0").has_value(), "contentVersion rejects 1.0.0",
           "");

    std::string error;
    report(!pandora::validate_title_id("CUSA12345", &error), "title id rejects PS4 range", error);
    report(pandora::validate_title_id("PPSA77010", &error), "title id accepts PPSA77010", "");

    const auto platform = pandora::make_platform();
    report(!platform->name().empty(), "platform available", platform->name());

    std::error_code fs_error;
    if (std::filesystem::is_directory(platform->data_root(), fs_error)) {
        report(true, "data root exists", platform->data_root());
    } else {
        // Not a failure: the server creates it at start-up.
        std::printf("[INFO] data root — %s will be created on first start\n",
                    platform->data_root().c_str());
    }

    std::printf("\n%d check(s) failed\n", failures);
    return failures == 0 ? kOk : kFailed;
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    pandora::set_log_level("warn");

    if (args.empty() || args[0] == "help" || args[0] == "--help" || args[0] == "-h") {
        print_usage();
        return args.empty() ? kUsage : kOk;
    }
    if (args[0] == "version" || args[0] == "--version" || args[0] == "-V") {
        std::printf("%s\n", kVersion);
        return kOk;
    }

    const std::string& command = args[0];

    if (command == "title-id") {
        if (args.size() < 3 || args[1] != "check") {
            return usage_error("usage: pandora-cli title-id check <TITLEID>");
        }
        return command_title_id_check(args[2]);
    }

    if (command == "param") {
        if (args.size() < 3) {
            return usage_error("usage: pandora-cli param <show|validate|bump> <file>");
        }
        const std::string& action = args[1];
        if (action == "show") {
            return command_param_show(args[2]);
        }
        if (action == "validate") {
            return command_param_validate(args[2]);
        }
        if (action == "bump") {
            return command_param_bump(args[2], args.size() > 3 ? args[3] : "");
        }
        return usage_error("unknown param action '" + action + "'");
    }

    if (command == "digest") {
        if (args.size() < 2) {
            return usage_error("usage: pandora-cli digest <file>");
        }
        return command_digest(args[1]);
    }

    if (command == "package") {
        if (args.size() < 3) {
            return usage_error("usage: pandora-cli package <dir> <out.zip>");
        }
        return command_package(args[1], args[2]);
    }

    if (command == "doctor") {
        return command_doctor();
    }

    return usage_error("unknown command '" + command + "'");
}
