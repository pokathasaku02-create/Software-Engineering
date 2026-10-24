# Coding standard

Based on the Google C++ Style Guide, simplified for this project.

| Topic | Rule |
|---|---|
| Language | C++17, no compiler extensions. Warnings are errors in review: the code must build cleanly with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`. |
| Naming | Types `PascalCase`; functions and methods `camelCase`; variables `camelCase`; private data members end with `_` (exception: the plain entity classes in `Entities.h`, which follow the class diagram); constants `kPascalCase`; namespace `lms`. |
| Files | One header + one source file per class (small related classes may share a pair). Header guard is `#pragma once`. Filenames match the main class name. |
| Layering | `ui` -> `service` -> `domain`; `service` -> `data` interfaces only. `ui` and `service` never include SQLite headers. Only `data/Database.cpp` includes `<sqlite3.h>`; only `service/PasswordHasher.cpp` includes `<sodium.h>`. |
| Memory | No raw `new`/`delete`. Use value types, `std::unique_ptr` and references. Ownership is shown in the type. |
| Errors | Signal errors with the `LmsException` hierarchy. Never `catch (...)` and ignore, except in destructors. Menus catch `LmsException` and `std::exception` so bad input cannot crash the program. |
| Const | Mark everything `const` that can be. Pass objects by `const&`; small values by value. Use `override` on every overriding method. |
| SQL | Parameterised statements only (`Statement::bind`). Never concatenate user input into SQL. |
| Secrets | Never store, log or print passwords. Only salted Argon2id hashes are stored. |
| Layout | 4-space indent, no tabs, lines up to ~120 characters, braces on the same line. |
| Comments | Explain *why*, not *what*. Each class gets a one-line purpose comment. |
| Tests | Every business rule has a test. Tests must not touch the real `lms.db`; use `":memory:"`. |
