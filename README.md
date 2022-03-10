# jemsys
Message passing subsystem/utilities for asynchronous computing (WIP)
**Currently undergoing major project restructuring**

Note: Abandonned as a cohesive project; has been redesigned as the branch off project [agate](https://github.com/maxwelliverson/agate), and I intend to do something with Silica at some point in the future as well.

# Motivation
WIP

# Subprojects
## Quartz
- Low level asynchronous kernel intended primarily for mediation of interprocess communication
## Agate
- Low level asynchronous communication primitives, optimized for process local communication 
(across threads, concurrent tasks, intraprocess actor models, etc.), but allowing for transparent interprocess communication. Built on top of Quartz.
## Agate2 (Will become primary project)
- Major redesign of Agate, allows for more flexiblity by transitioning to a handle based API.
## Opal (Abandonned for now)
- Modern C++ API built on top of Agate
## Silica 
- Library for creating, storing, and modifying modules containing data type specifications, intended for builtin integration with Agate for the creation of actor-like models.
## Silica2
- Major redesign/refactoring of Silica to fit with Agate2 instead.
