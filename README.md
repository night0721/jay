# jay

jay is a single header JSON library for C, there is only two functions, `jay_parse` and `jay_print` which parse and print JSON respectively.

# Usage
You must define `JAY_IMPLEMENTATION` before including the header file in one of your source files.

To use `jay_parse`, a `char *` to the JSON object is passed and a `json_value` is returned which contains the parsed JSON object.
`jay_print` takes a `json_value` and `indent` which is the number of spaces to indent the JSON object, and prints the JSON object to stdout.

Simple example can be found in [example.c](https://github.com/night0721/jay/blob/master/example.c).

# Building
```
$ cc jay.h example.c -o example
```

# Contributions
Contributions are welcomed, feel free to open a pull request.

# License
This project is licensed under the GNU Public License v3.0. See [LICENSE](https://github.com/night0721/jay/blob/master/LICENSE) for more information.
