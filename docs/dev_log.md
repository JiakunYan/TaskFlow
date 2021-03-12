# Task Framework Development Log

**Task Dependency Graph**: DAG

**How do we store a task dependency graph?** TaskTorrent stores it as a function.

**Task Dependency Type**:
- control dependency
- data dependency: data dependency can be viewed as a special control dependency.

**completion detection**:
completion detection is not trivial.

### Questions:
- Can `Taskflow` be a template class?
- Should we support task `yield`? 
  We need Argobots to support `yield`.