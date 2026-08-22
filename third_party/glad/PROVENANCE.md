# GLAD2 provenance

The checked-in loader was generated from GLAD2 v2.0.8, commit
`73db193f853e2ee079bf3ca8a64aa2eaf6459043`.

Generation command (from an isolated Python environment containing that revision):

```text
python -m glad --out-path <output> --api gl:core=4.6 --extensions "" --reproducible c
```

No built-in loader was generated because GLFW supplies `glfwGetProcAddress`.
