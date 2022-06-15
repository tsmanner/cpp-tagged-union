const std = @import("std");
const GitRepo = @import("build-lib/GitRepo.zig");
const Bash = @import("build-lib/Bash.zig");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    // Make sure that any external dependencies use zig as their C/C++ compiler.
    b.env_map.put("CC", "zig cc") catch @panic("memory");
    b.env_map.put("CXX", "zig c++") catch @panic("memory");

    const catch_repo = GitRepo.create(b, .{
        .url = "https://github.com/catchorg/Catch2",
        .branch = "v3.0.0-preview5",
        .path = "tests/support/Catch2",
        .sha = "f526ff0fc37ae00ff2c0dc8f6550bf8209c30afc",
    });

    const catch_build = Bash.create("Build Catch2", b,
        \\if ! [ -e tests/support/Catch2/build ]
        \\  then
        \\  mkdir tests/support/Catch2/build
        \\fi
        \\cd tests/support/Catch2/build
        \\cmake -G Ninja ..
        \\ninja Catch2WithMain
    );
    catch_build.step.dependOn(&catch_repo.step);

    //
    // Profiling executable
    //
    const profiling = b.addExecutable("profiling", null);
    profiling.setTarget(target);
    profiling.setBuildMode(mode);
    profiling.linkLibCpp();
    profiling.force_pic = true;
    // Set up dependency on Catch2
    // profiling.step.dependOn(&catch_build.step);
    // profiling.addLibPath(catch_repo.getRelativePath(&profiling.step, "build/src"));
    // profiling.linkSystemLibrary("Catch2");
    // profiling.linkSystemLibrary("Catch2Main");
    // profiling.addIncludeDir(catch_repo.getRelativePath(&profiling.step, "build/generated-includes"));
    // profiling.addIncludeDir(catch_repo.getRelativePath(&profiling.step, "src"));
    profiling.addIncludeDir("include");
    // Add the test source files
    profiling.addCSourceFiles(&.{
        "profiling/access_time.cpp",
    }, &.{
        "-std=c++14",
        "-Wall",
        "-Werror",
    });

    const run_profiling = profiling.run();
    run_profiling.step.dependOn(&profiling.step);
    if (b.args) |args| {
        run_profiling.addArgs(args);
    }

    const run_profiling_step = b.step("profile", "Report some profiling statistics");
    run_profiling_step.dependOn(&run_profiling.step);

    //
    // Simple main executable
    //
    const main_exe = b.addExecutable("main", null);
    main_exe.setTarget(target);
    main_exe.setBuildMode(mode);
    main_exe.linkLibCpp();
    main_exe.force_pic = true;
    main_exe.addIncludeDir("include");
    // Add the test source files
    main_exe.addCSourceFiles(&.{
        "main.cpp",
    }, &.{
        "-std=c++14",
        "-Wall",
        "-Werror",
    });

    const run_main = main_exe.run();
    run_main.step.dependOn(&main_exe.step);
    if (b.args) |args| {
        run_main.addArgs(args);
    }

    const run_main_step = b.step("main", "Report some main statistics");
    run_main_step.dependOn(&run_main.step);
}
