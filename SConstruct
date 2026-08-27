#!/usr/bin/env python
import os
import sys

from methods import print_error


libname = "godot-recycler-view"
projectdir = "project"

localEnv = Environment(tools=["default"], PLATFORM="")

# Build profiles can be used to decrease compile times.
# You can either specify "disabled_classes", OR
# explicitly specify "enabled_classes" which disables all other classes.
# Modify the example file as needed and uncomment the line below or
# manually specify the build_profile parameter when running SCons.

# localEnv["build_profile"] = "build_profile.json"

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

# Standalone unit test runner for the pure-algorithm layer (no Godot runtime).
# Usage: scons tests=yes
if ARGUMENTS.get("tests", "no") == "yes":
    test_env = Environment(tools=["default"], PLATFORM="")
    test_env.Append(CPPPATH=[
        "godot-cpp/include",
        "godot-cpp/gen/include",
        "godot-cpp/gdextension",
        "src/",
        "tests/",
    ])
    test_env.Append(CXXFLAGS=["-std=c++17", "-O0", "-g"])
    test_sources = Glob("tests/*.cpp") + ["src/diff_algo.cpp", "src/op_reorderer.cpp", "src/update_op_apply.cpp", "src/layout_math.cpp", "src/fling_scroller.cpp", "src/velocity_tracker.cpp"]
    runner = test_env.Program("tests/bin/test_runner", source=test_sources)
    Default(runner)
    Return()

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

# .dev doesn't inhibit compatibility, so we don't need to key it.
# .universal just means "compatible with all relevant arches" so we don't need to key it.
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), libname, suffix, env.subst('$SHLIBSUFFIX'))

library = env.SharedLibrary(
    "bin/{}/{}".format(env['platform'], lib_filename),
    source=sources,
)

copy = env.Install("{}/bin/{}/".format(projectdir, env["platform"]), library)

default_args = [library, copy]
Default(*default_args)
