#!/usr/bin/python3

Import("env")
import os
import shutil

def before_build():
    print("=====================================")
    print("Running cleanup_libdeps.py script")
    
    # Paths to the example directories we want to remove
    example_paths = [
        os.path.join(env.subst("$PROJECT_DIR"), ".pio", "libdeps", "teensy40", "teensy-4-async-inputs", "example1"),
        os.path.join(env.subst("$PROJECT_DIR"), ".pio", "libdeps", "teensy40", "teensy-4-async-inputs", "exampleESP32")
    ]
    
    for example_path in example_paths:
        print("Checking path:", example_path)
        if os.path.exists(example_path):
            print(f"Removing {os.path.basename(example_path)} directory...")
            shutil.rmtree(example_path)
        else:
            print(f"Directory already removed: {example_path}")
    
    print("=====================================")
    return ""

env.Append(
    BUILD_FLAGS=[before_build()]
)