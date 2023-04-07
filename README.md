# Procedural-Parametric Paradigm Compiler

This is a C metacompiler, which adds native support for fancy linked unions.

A thorough explanation (in English): https://cortan122.tk:80/latex_pdfs/курсач_pp.pdf \
Original idea (in Russian): http://softcraft.ru/paradigm/ppp/

## Build & Run

```console
$ make
$ ./main "test inputs/example.c"
```

## Install pre-commit hooks

```console
$ pip install pre-commit
$ pre-commit install
```

## Run tests

```console
$ make run_tests
```
