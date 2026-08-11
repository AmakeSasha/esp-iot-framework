# 
# SPDX-License-Identifier: Apache-2.0
# Project: esp-iot-framework
# Folder: ./tools
# File: check_header.py
# 
# Copyright 2026 AmakeSasha
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from pathlib import Path

PROJECT_NAME = "esp-iot-framework"
PROJECT_COPYRIGHT = "Copyright 2026 AmakeSasha"
START_DIR = "../"

# ---------- CONSTS ----------
BAN_LIST = {
    ".gz",
    ".png",
    ".yml",
    ".yaml",
    "LICENSE",
    ".cppcheck",
    "../NOTICE",
    ".gitignore",
    "dependencies.lock",
    "../docs/doxygen/header.html",

    "/.git/",
    "/build/",
    "/docs/html/",
    "/.devcontainer/",
    "/esp-iot-framework_cppcheck/",
}

LICENSE = {
    "MIT": [
        "MIT License",
        "",
        "Permission is hereby granted, free of charge, to any person obtaining a copy",
        "of this software and associated documentation files (the \"Software\"), to deal",
        "in the Software without restriction, including without limitation the rights",
        "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell",
        "copies of the Software, and to permit persons to whom the Software is",
        "furnished to do so, subject to the following conditions:",
        "",
        "The above copyright notice and this permission notice shall be included in",
        "all copies or substantial portions of the Software.",
        "",
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR",
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,",
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE",
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER",
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,",
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE",
        "SOFTWARE."
    ],
    "Apache-2.0": [
        "Licensed under the Apache License, Version 2.0 (the \"License\");",
        "you may not use this file except in compliance with the License.",
        "You may obtain a copy of the License at",
        "",
        "    http://www.apache.org/licenses/LICENSE-2.0",
        "",
        "Unless required by applicable law or agreed to in writing, software",
        "distributed under the License is distributed on an \"AS IS\" BASIS,",
        "WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.",
        "See the License for the specific language governing permissions and",
        "limitations under the License."
    ],
    "CC0-1.0": [
        "Public Domain"
    ]
}
# ---------- END CONSTS ----------


# ---------- ERRORS -----------
def err_incorrect_header(line_num, line):
    print(f"<--- Incorrect header format: [{line_num}] '{line}'")
    return False

def err_missing_header(line_num, header):
    print(f"<--- Missing header: [{line_num}] '{header}'")
    return False

def err_invalid_copyright(value, need_value):
    print(f"<--- Invalid copyright: '{value}', need '{need_value}'")
    return False

def err_invalid_field_value(header, value, need_value):
    if need_value == None:
        print(f"<--- Invalid header value: '{header}' is '{value}'")
    else:
        print(f"<--- Invalid header value: '{header}' is '{value}', need: '{need_value}'")
    return False
# ---------- ENDERRORS -----------



# ---------- CHECKERS -----------
def check_header_format(f, type_file):
    lines = [f.readline().rstrip("\n") for _ in range(35)]
    
    lines = check_comment_format(lines, type_file)
    if lines is False or lines is None:
        return print("<--- Comment format validation failed.")

    metadata = tokenize_header(lines)
    if metadata is False or metadata is None:
        return print("<--- Tokenization failed.")

    check_metadata(Path(f.name), metadata, lines)

def check_comment_format(lines, type_file):
    return_lines = []
    format_comments = []
    start_range = 0
    last_line_num = lines.index("")

    if (type_file == "c_style"):
        format_comments = ["/*", " * ", " */"]
    elif (type_file == "html"):
        format_comments = ["<!--", "  ", "-->"]
    elif (type_file == "hash"):
        format_comments = [None, "# ", None]
    else:
        return

    if format_comments[0] != None:
        start_range = 1

        if lines[0] != format_comments[0]:
            return err_incorrect_header(0, lines[0])

    if format_comments[2] != None:
        last_line_num -= 1

        if lines[last_line_num] != format_comments[2]:
            return err_incorrect_header(last_line_num, lines[last_line_num])

    for num in range(start_range, last_line_num):
        if not lines[num].startswith(format_comments[1]):
            return err_incorrect_header(num, lines[num])

        return_lines.append(lines[num][len(format_comments[1]):])

    if (len(return_lines[0]) == 0):
        del return_lines[:1]

    return return_lines

def check_metadata(path, metadata, lines):
    folder_path = ""
    file_name = path.name
    raw_parent = path.parent.as_posix()

    if raw_parent == "..":
        folder_path = "."
    else:
        folder_path = f"./{raw_parent.replace('../', '')}"

    if (len(lines[0]) == 0):
        del lines[:1]

    if (metadata.get("Project") == PROJECT_NAME):
        if (not check_copyright(lines, metadata, True)):
            return False

        if "./components/" in folder_path:
            component_name = folder_path.split("/")[2]
            
            if component_name != metadata["Library"]:
                return err_invalid_field_value("Library", metadata["Library"], component_name)
   
    elif (metadata.get("Library") != None):
        if (not check_copyright(lines, metadata, False)):
            return False

        relative_folder = metadata.get("Relative folder")
        if (relative_folder != None):

            relative_folder = relative_folder.split("/")[1:]
            component_name = folder_path.split("/")[:3]
            absolute_folder = "/".join(component_name + relative_folder)

            if (absolute_folder != folder_path):
                need_relative_folder = "./" + "".join(folder_path.split("/", 3)[3:])
                return err_invalid_field_value(
                    "Relative folder", metadata["Relative folder"], need_relative_folder
                )

    elif (metadata.get("Example") != None):
        if (not check_copyright(lines, metadata, True)):
            return False

        if not "./examples/" in metadata["Folder"]:
            return err_invalid_field_value("Folder", metadata["Folder"], folder_path)

    else:
        return err_invalid_field_value("Project/Library", None, None)

    if (metadata["Folder"].lower() != folder_path.lower()):
        return err_invalid_field_value("Folder", metadata["Folder"], folder_path)
    if (metadata["File"].lower() != file_name.lower()):
        return err_invalid_field_value("File", metadata["File"], file_name)

    license = metadata["SPDX-License-Identifier"]

    if LICENSE[license] != lines:
        return err_invalid_field_value("SPDX-License-Identifier", license, None)

    return True

def check_copyright(lines, metadata, is_my):
    if (metadata["SPDX-License-Identifier"] == "CC0-1.0"):
        return True

    copyright = lines.pop(0)

    if (is_my):
        if (copyright != PROJECT_COPYRIGHT):
            return err_invalid_copyright(copyright, PROJECT_COPYRIGHT)
        del lines[:1]
    else:
        author = metadata["Original Author"]
        if (not author in copyright):
            return err_invalid_copyright(copyright, f"Copyright <DATA> {author}")
        del lines[:1]
        
    return True
# ---------- END CHECKERS -----------



# ---------- PARSERS ----------
def tokenize_header(lines):
    metadata = {}
    num_lines = 0
    
    for num in range(0, len(lines)):
        line = lines[num]
        
        if line == "":
            break

        if ": " in line:
            key, value = line.split(": ", 1)
            metadata[key] = value
            num_lines += 1
        else:
            return err_incorrect_header(num, line)

    del lines[:num_lines + 1]

    return metadata

def get_comment_type(path):
    COMMENT_HTML    = { ".md", ".html", ".xml" }
    COMMENT_C_STYLE = { ".h", ".c", ".js", ".css", ".cpp" }
    COMMENT_HASH    = {
        ".py",
        "kconfig",
        "doxyfile",
        "cmakelists.txt",
        "kconfig.projbuild",
        "sdkconfig.defaults"
    }

    file_name = path.name.lower()
    file_ext = path.suffix.lower()

    if any(ext in file_name for ext in COMMENT_HASH):
        return "hash"
    elif file_ext in COMMENT_C_STYLE:
        return "c_style"
    elif file_ext in COMMENT_HTML:
        return "html"
    else:
        return ""

def scan_directory(root_dir, ban_list):
    base_path = Path(root_dir)

    for path in base_path.rglob("*"):
        if path.is_file():
            posix_path = path.as_posix()

            if any(ban_item in posix_path for ban_item in ban_list):
                continue

            print(f"Opening file: {posix_path}")

            try:
                with open(posix_path, "r", encoding="utf-8", errors="ignore") as f:
                    type_file = get_comment_type(path)

                    if (type_file == ""):
                        print(f"<--- Unknown type file: {posix_path}")
                    else:
                        check_header_format(f, type_file)
            except Exception as e:
                print(f"<--- Error reading {posix_path}: {e}")
# ---------- END PARSERS ----------

if __name__ == "__main__":
    scan_directory(START_DIR, BAN_LIST)