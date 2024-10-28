# About

Scripny is a tui tool that helps users navigate, search, and execute shell scripts within the `scripts/` directory present in the same directory as the program. With a simple interface, it allows you to quickly find and run the scripts you need.

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Contributing](#contributing)
- [License](#license)

## Features

- **Navigate**: Easily browse through your scripts directory.
- **Search**: Quickly find scripts based on their names.
- **Execute**: Run your scripts directly from the tool and check if it was successful or not.

## Requirements

- C compiler like GCC.
- Terminal.

## Installation

To build Scripny, ensure you have a c compiler (like gcc) installed, and then run the following command in your terminal:
```shell
gcc -Wall -Wextra -O2 scripny.c -o scripny
```

> Add the `-D_DEFAULT_SOURCE` flag if compilation returns error related to `strdup`, `DT_DIR` or `DT_REG`

This will compile the source code into an executable named `scripny`.

## Usage

> Make sure you have scripts with `.sh` extension in the `scripts/` directory where `scripny` is. Otherwise it will just show a blank menu.  

1. **Run Scripny**:
    
   ```shell
   ./scripny
   ```
   
2. **Navigate Scripts**:
    Press `j` and `k` keys to navigate through the scripts.

2. **Filter Scripts**:
    Press `/` and enter desired filter term to filter scripts based on it. Press `esc` to cancel it.

3. **Execute a Script**:
    Press `x` to execute the selected script.

## Configuration

- **Changing the scripts directory**:
  You can change the scripts directory by simply changing the value of 'SCRIPT_PATH' macro in the scripny.c file.
  
  > **Note:** Path is relative to where the program is executed.

## Contributing

If you have suggestions for improvements or bug fixes, feel free to create a pull request or open an issue.

## License

This project is licensed under the MIT License. See the LICENSE file for more details.
