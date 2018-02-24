"""
symlinks.py

Creates a set of symbolic links in the default sketchbook directory for
all Arduino libraries in this repository. The purpose is to make it easy to
install a set of Arduino libraries.

symlinks.py usage:

    python symlinks.py --install  # Creates or refreshes links to the libraries
    python symlinks.py --remove   # Removes links to the libraries

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


def get_default_path():
    USER_DIR = os.path.expanduser('~')
    if platform.system() == 'Linux':
        ARDUINO_DIR = os.path.join(USER_DIR, 'Arduino')
    elif platform.system() == 'Darwin':
        ARDUINO_DIR = os.path.join(USER_DIR, 'Documents', 'Arduino')
    elif platform.system() == 'Windows':
        sys.exit(0)  # Windows not supported

    return ARDUINO_DIR


def create_symlinks(ARDUINO_DIR, subdir):
    ACTUAL_DIR = os.path.join(ARDUINO_DIR, subdir)  # Make it generic so we can symlink Arduino/libraries as well as Arduino/hardware

    # Create directory if it doesn't exist
    if not os.path.isdir(ACTUAL_DIR):
        print('Arduino "{}" directory does not exist - Creating'.format(subdir))
        os.makedirs(ACTUAL_DIR)

    # Update all libraries (using git submodule)
    print('Making sure you have the latest version of each submodule/library...')
    call(['git', 'submodule', 'init'])
    call(['git', 'submodule', 'update'])
    call(['python', 'get.py'], cwd='hardware/esp8266com/esp8266/tools')  # Esp8266 git repo also requires a manual download of the tools (won't re-download if already downloaded)
    print('All submodules updated :)')

    # Create symbolic links
    src_paths, dst_paths = get_src_and_dst_paths(ARDUINO_DIR, subdir)
    for src, dst in zip(src_paths, dst_paths):
        if os.path.exists(dst):  # If dst library folder already exists, decide between:
            if not os.path.islink(dst):  # If the folder is not a symlink and already existed, leave it as is
                print('{} exists and is not a symbolic link - not overwriting'.format(dst))
                continue
            else:  # If it was a symlink, just "refresh" (update) it
                print('\tUnlinking {} first'.format(dst))
                os.unlink(dst)
        # Create symbolic link
        print('Creating new symbolic link {}'.format(dst))
        os.symlink(src, dst)

    print('Done! :)')


def remove_symlinks(ARDUINO_DIR, subdir):
    ACTUAL_DIR = os.path.join(ARDUINO_DIR, subdir)  # Make it generic so we can symlink Arduino/libraries as well as Arduino/hardware

    # If library directory doesn't exist there's nothing to do
    if not os.path.isdir(ACTUAL_DIR):
        return

    # Remove symbolic links
    src_paths, dst_paths = get_src_and_dst_paths(ARDUINO_DIR, subdir)
    for dst in dst_paths:
        if os.path.islink(dst):
            print('Removing symbolic link {}'.format(dst))
            os.unlink(dst)

    print('Done! :)')


def get_src_and_dst_paths(ARDUINO_DIR, subdir):
    """
    Get source and destination paths for symbolic links
    """
    cur_dir = os.path.abspath(os.path.curdir)
    actual_dir = os.path.join(cur_dir, subdir)  # Make it generic so we can symlink Arduino/libraries as well as Arduino/hardware
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


# -----------------------------------------------------------------------------
if __name__ == '__main__':
    SYMLINK_FOLDERS = ['libraries', 'hardware']  # Create symlinks to Arduino/libraries and Arduino/hardware

    parser = argparse.ArgumentParser(description='Arduino Libraries Symlinks')
    parser.add_argument('-i', '--install',
                        help='Install all of the Arduino libraries in this repository into Arduino through symbolic links.',
                        action='store_true')
    parser.add_argument('-r', '--remove',
                        help='Remove all of the Arduino library symbolic links from the Arduino libraries directory.',
                        action='store_true')
    parser.add_argument('-p', '--path',
                        help='Path to the Arduino library folder (optional, by default [{}] will be used).'.format(get_default_path()),
                        default=get_default_path())

    args = parser.parse_args()
    if args.install:
        for dir in SYMLINK_FOLDERS:
            create_symlinks(args.path, dir)
    elif args.remove:
        for dir in SYMLINK_FOLDERS:
            remove_symlinks(args.path, dir)
    else:  # If neither install nor remove actions, print help
        parser.print_help()
