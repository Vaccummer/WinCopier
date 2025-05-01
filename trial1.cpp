#include <CLI11.hpp>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv)
{

    CLI::App app{"AMIO"};
    bool exat;
    app.add_flag("-e,--exact", exat, "Make dir when dst dir not exists");

    bool use_regex = false;
    bool mkdir = false;
    bool conflict_newname = false;
    bool force_overlap = false;
    bool quiet = false;
    bool permanent_delete = false;

    std::vector<std::string> cp_paths{};

    CLI::App *copy_cmd = app.add_subcommand("cp", "Copy path to a certain directory");

    copy_cmd->add_option("Srcs&Dst", cp_paths, "If only one path, copy to cwd or the last one serves as dst dir")
        ->required();

    copy_cmd->add_flag("-m,--mkdir", mkdir, "Make dir when dst dir not exists");

    copy_cmd->add_flag("-f,--force", force_overlap, "Force overlap path when dst path already exists");

    copy_cmd->add_flag("-n,--new", conflict_newname, "Create new name when dst path already exists ");

    copy_cmd->add_flag("-r,--regex", use_regex, "User regex to find paths, use <> to wrap your pattern");

    copy_cmd->add_flag("-q,--quiet", quiet, "No UI, auto cre new name when conflict");

    std::vector<std::string> cl_paths;
    CLI::App *clone_cmd = app.add_subcommand("cl", "Clone src to dst");
    clone_cmd->add_option("Src&Dst", cl_paths, "Target Path and Destination")
        ->required()
        ->expected(2);
    clone_cmd->add_flag("-m,--mkdir", mkdir, "Make dir when dst dir not exists");
    clone_cmd->add_flag("-f,--force", force_overlap, "Force overlap path when dst path already exists");
    clone_cmd->add_flag("-n,--new", conflict_newname, "Create new name when dst path already exists");
    clone_cmd->add_flag("-q,--quiet", quiet, "No UI, auto cre new name when conflict");

    std::vector<std::string> mr_paths{};
    CLI::App *replace_cmd = app.add_subcommand("mr", "Move and Replace");
    replace_cmd->add_option("Src&Dst", mr_paths, "Move the fist path to second's upper dir and replace it")
        ->required()
        ->expected(2);
    replace_cmd->add_flag("-m, --mkdir", mkdir, "Make dir when dst dir not exists");
    replace_cmd->add_flag("-f,--force", force_overlap, "Force overlap path when dst path already exists");
    replace_cmd->add_flag("-n,--new", conflict_newname, "Create new name when dst path already exists");
    replace_cmd->add_flag("-q,--quiet", quiet, "No UI, auto cre new name when conflict");

    std::vector<std::string> rm_paths;
    CLI::App *remove_cmd = app.add_subcommand("rm", "Remove paths");
    remove_cmd->add_option("Paths", rm_paths, "The paths to be removed")
        ->required();
    remove_cmd->add_flag("-r,--regex", use_regex, "Use regex to find paths, use <> to wrap your pattern");
    remove_cmd->add_flag("-q,--quite", quiet, "Use regex to find paths, use <> to wrap your pattern");
    remove_cmd->add_flag("-p,--permanent", use_regex, "Directly delete path rather than move to Recycle Bin (But UNDO is still available)");

    std::vector<std::string> rn_paths;
    CLI::App *rename_cmd = app.add_subcommand("rename", "Rename path to a new name");
    rename_cmd->add_option("src&newname", rn_paths, "Move the fist path to second's upper dir and replace it")
        ->required()
        ->expected(2);
    rename_cmd->add_flag("-f,--force", force_overlap, "Force overlap path when dst path already exists");

    std::vector<std::string> new_paths;
    CLI::App *new_cmd = app.add_subcommand("new", "Create a new file");
    new_cmd->add_option("dsts", new_paths, "The paths to be removed")
        ->required();
    new_cmd->add_flag("-m,--mkdir", mkdir, "Make dir when dst dir not exists");

    try
    {
        CLI11_PARSE(app, argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        return app.exit(e);
    }

    return 0;
}
