# lab4

Консольный конвертер между JSON, TOML и XML на C++23. Вход читается из stdin, результат пишется в stdout. Преобразование идёт через общее абстрактное дерево.

## Сборка

Нужны CMake >= 3.25 и компилятор с поддержкой C++23.

```sh
cmake -B build
cmake --build build
```

Опции: `-DLAB4_BUILD_TESTS=OFF` — не собирать тесты (GoogleTest), `-DLAB4_WERROR=ON` — предупреждения как ошибки.

## Использование

```sh
./build/src/lab4 <json|toml|xml> <json|toml|xml> < input > output
# пример
echo '{"a": 1, "b": [true, 1.5]}' | ./build/src/lab4 json toml
```

Коды возврата: `0` — успех, `1` — неверные аргументы, `2` — ошибка разбора (с указанием строки), `3` — ошибка преобразования.

## Ограничения

- Типы: object, array, string, int (int64), float, bool. `null` и даты TOML не поддерживаются.
- Только ASCII; ключи вида `[A-Za-z_][A-Za-z0-9_-]*`.
- Корень документа TOML должен быть таблицей; глубина вложенности ≤ 256.

## Тесты

```sh
ctest --test-dir build
```
