#!/usr/bin/env python3

import os
import subprocess
import argparse


def error(msg=""):
  print("error: %s" % msg)
  exit(1)


def check_yapf():
  # check yapf exist
  os.system("yapf --version") == 0 or os.system(
    "pip install yapf"
  ) == 0 or error(
    "error: fail to use yapf, maybe you can try install by command: pip install yapf --user"
  )


def get_root_path(anchor=".clang-format"):
  path = os.path.abspath(__file__)
  while True:
    path = os.path.dirname(path)
    if (os.path.exists(path + "/" + anchor)):
      return path
    if (path == "/"):
      error("%s not found" % anchor)


def _run_cpp(sub_folders,
             file_types=["*.cpp", "*.c", "*.hpp", "*.h"],
             format_file=".clang-format"):
  assert isinstance(file_types, list)
  assert format_file.strip()
  root = get_root_path(format_file)
  print("check in [%s] with [%s]" %
        (", ".join(sub_folders), ", ".join(file_types)))
  for folder in sub_folders:
    for file_type in file_types:
      cmd = "find %s/%s -name %s | xargs clang-format -i 2>/dev/null" % (
        root, folder, file_type)
      os.system(cmd)


def run():
  sub_folders = ["../src", "../include"]
  _run_cpp(sub_folders)

def _check_cpp(sub_folders,
               file_types=["*.cpp", "*.c", "*.hpp", "*.h"],
               format_file=".clang-format"):
  assert isinstance(file_types, list)
  assert format_file.strip()
  root = get_root_path(format_file)
  print("check in [%s] with [%s]" %
        (", ".join(sub_folders), ", ".join(file_types)))
  for folder in sub_folders:
    for file_type in file_types:
      try:
        cmd = "find %s/%s -name %s | xargs clang-format -output-replacements-xml | grep -c '<replacement '" % (
          root, folder, file_type)
        print("cmd = %s" %cmd)
        result = subprocess.check_output(cmd, shell=True)
        if int(result) > 10:
          error("%s of %s/%s need to formatted" % (int(result), root, folder))
      except Exception as e:
        continue

def check():
  sub_folders = ["../src", "../include"]
  _check_cpp(sub_folders)

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("action",
                      choices=["check", "run"],
                      nargs="?",
                      default="run",
                      help="The actions")
  args = parser.parse_args()
  if (args.action == "run"):
    run()
  elif (args.action == "check"):
    check()
  exit(0)
