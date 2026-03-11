# nested repos guide

Nested repositories (for example Moqui framework style layouts) are supported.
Each repo is still defined as a separate entry in `mrm.yml` with a nested `name`.

## example layout

```
work/
  moqui/
  moqui/runtime/
  moqui/runtime/component/mantle-usl/
  moqui/runtime/component/mantle-udm/
  moqui/runtime/component/SimpleScreens/
```

## generate config

```sh
mrm find work --save
```

This will generate nested repo entries like:

```yaml
name: moqui
name: moqui/runtime
name: moqui/runtime/component/mantle-usl
name: moqui/runtime/component/mantle-udm
name: moqui/runtime/component/SimpleScreens
```

## operating safely

- Use `mrm status` to verify each nested repo is clean.
- Use `--root` patterns to isolate a subtree when needed.
- Avoid aggressive pruning unless your config is fully accurate.
