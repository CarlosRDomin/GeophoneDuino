"""
setup.py

Creates a set of symbolic links in the default sketchbook directory for
all Arduino libraries in this repository. The purpose is to make it easy to
install a set of Arduino libraries.

setup.py usage:

    python setup.py --install  # Creates or refreshes links to the libraries
    python setup.py --remove   # Removes links to the libraries

Original authors:
Will Dickson will@iorodeo.com
Peter Polidoro polidorop@janelia.hhmi.org

Modified by:
Carlos Ruiz carlos.r.domin@gmail.com
"""

import sys
import os
import argparse
import platform
from subprocess import call
from log_helper import logger


def get_default_Arduino_path():
    USER_DIR = os.path.expanduser('~')
    if platform.system() == "Linux":
        ARDUINO_DIR = os.path.join(USER_DIR, "Arduino")
    elif platform.system() == "Darwin":
        ARDUINO_DIR = os.path.join(USER_DIR, "Documents", "Arduino")
    else:  # if platform.system() == "Windows":
        sys.exit(0)  # Windows not supported

    return ARDUINO_DIR


def get_src_and_dst_paths(ARDUINO_DIR, subdir):
    """
    Get source and destination paths for symbolic links
    """
    cur_dir = os.path.abspath(os.path.curdir)
    actual_dir = os.path.join(cur_dir, "submodules", subdir)  # Make it generic so we can symlink Arduino/libraries as well as Arduino/hardware
    dir_list = os.listdir(actual_dir)  # Enumerate all folders in the current folder (ie, all libraries that need to be symlinked)

    src_paths = []
    dst_paths = []
    for item in dir_list:
        if os.path.isdir(os.path.join(actual_dir, item)):  # Traverse all folders
            if item[0] == '.':   # Ignore .git folders and so on (just in case)
                continue
            src_paths.append(os.path.join(actual_dir, item))
            dst_paths.append(os.path.join(ARDUINO_DIR, subdir, item))
    return src_paths, dst_paths


def create_symlinks(ARDUINO_DIR, subdir):
    ACTUAL_DIR = os.path.join(ARDUINO_DIR, subdir)  # Make it generic so we can symlink Arduino/libraries as well as Arduino/hardware

    # Create directory if it doesn't exist
    if not os.path.isdir(ACTUAL_DIR):
        logger.debug("Arduino '{}' directory does not exist - Creating".format(subdir))
        os.makedirs(ACTUAL_DIR)

    # Update all libraries (using git submodule)
    logger.notice("Making sure you have the latest version of each submodule/library...")
    call(["git", "submodule", "init"])
    call(["git", "submodule", "update"])
    logger.success("All submodules updated :)")

    # Create symbolic links
    src_paths, dst_paths = get_src_and_dst_paths(ARDUINO_DIR, subdir)
    for src, dst in zip(src_paths, dst_paths):
        if os.path.exists(dst):  # If dst library folder already exists, decide between:
            if not os.path.islink(dst):  # If the folder is not a symlink and already existed, leave it as is
                logger.warning("{} exists and is not a symbolic link - not overwriting".format(dst))
                continue
            else:  # If it was a symlink, just "refresh" (update) it
                logger.verbose("Unlinking {} first".format(dst))
                os.unlink(dst)
        # Create symbolic link
        logger.debug("Creating new symbolic link {}".format(dst))
        os.symlink(src, dst)

    logger.success("Done! :)")
    return True


def remove_symlinks(ARDUINO_DIR, subdir):
    ACTUAL_DIR = os.path.join(ARDUINO_DIR, subdir)  # Make it generic so we can symlink Arduino/libraries as well as Arduino/hardware

    # If library directory doesn't exist there's nothing to do
    if not os.path.isdir(ACTUAL_DIR):
        return

    # Remove symbolic links
    src_paths, dst_paths = get_src_and_dst_paths(ARDUINO_DIR, subdir)
    for dst in dst_paths:
        if os.path.islink(dst):
            logger.debug("Removing symbolic link {}".format(dst))
            os.unlink(dst)

    logger.success("Done! :)")


def download_hardware_tools():
    # Download esp8266 tools (not part of the repo so `git submodule update` won't fetch them)
    logger.notice("Downloading additional esp8266 tools not included in the repo...")
    try:
        esp_tools_folder = os.path.abspath("submodules/hardware/esp8266com/esp8266/tools")
        cur_dir = os.path.abspath(os.curdir)  # Save a copy of the current dir so we can return after the script is run
        os.chdir(esp_tools_folder)  # Enter the directory
        sys.path.insert(0, esp_tools_folder)  # Add it to the PATH so get.py can be imported
        import get as esp_get  # Import the script as a module

        # Perform the same steps as the "if __name__ == '__main__':" block
        esp_get.mkdir_p(esp_get.dist_dir)
        tools_to_download = esp_get.load_tools_list("../package/package_esp8266com_index.template.json", esp_get.identify_platform())
        for tool in tools_to_download:
            esp_get.get_tool(tool)

        # Finally, remember to return to the original folder (so the rest of our script works)
        os.chdir(cur_dir)
    except Exception as e:
        logger.critical("ERROR downloading esp8266 tools. Reason: {}".format(e))
        return False
    logger.success("Additional esp8266 tools downloaded :)")
    return True


def install_plugins(ARDUINO_DIR):
    logger.notice("Installing the SPIFFS (filesystem) plugin...")
    call(["./make.sh"], env=dict(os.environ, INSTALLDIR=ARDUINO_DIR), cwd="submodules/plugins/ESP8266FS")
    logger.notice("Hopefully everything went right [check the output above] ;)")


# -----------------------------------------------------------------------------
if __name__ == "__main__":
    SYMLINK_FOLDERS = ["libraries", "hardware"]  # Create symlinks to Arduino/libraries and Arduino/hardware

    parser = argparse.ArgumentParser(description="GeophoneDuino Setup: downloads and symlinks necessary 3rd party libraries/tools")
    parser.add_argument("-i", "--install",
                        help="Install the 3rd party libraries into Arduino through symbolic links.",
                        action="store_true")
    parser.add_argument("-r", "--remove",
                        help="Remove the 3rd party library symbolic links from the Arduino libraries directory.",
                        action="store_true")
    parser.add_argument("-p", "--path",
                        help="Path to the Arduino root folder (optional, by default [%(default)s] will be used).",
                        default=get_default_Arduino_path())

    args = parser.parse_args()
    if args.install:
        # Create symlinks for every submodule folder (libraries, hardware...)
        for dir in SYMLINK_FOLDERS:
            if not create_symlinks(args.path, dir):  # Stop the process as soon as a component fails
                logger.critical("Installation failed! :( Please see errors above and fix them before trying again.")
                exit(-1)
        # And perform additional steps (install esp8266 tools and SPIFFS plugin)
        download_hardware_tools()
        install_plugins(args.path)
    elif args.remove:
        # Remove symlinks for every submodule folder (libraries, hardware...)
        for dir in SYMLINK_FOLDERS:
            remove_symlinks(args.path, dir)
    else:  # If neither install nor remove actions, print help
        parser.print_help()
