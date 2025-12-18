# OSAM Cache

This repository implements a "cache" abstraction on top of SAM and "raw" smart pointers, that we call Cached SAM. The client interface of Cached SAM involves two types: `CacheObj` and `CachePtr`. Using these types, clients can write SAM programs that are intuitive, guaranteed to be legal with respect to the SAM properties (never read/write an address more than once), and efficient. Importantly, this completely abstracts away the underlying SAM and smart pointer semantics from the user. We describe the components in more detail below:

## The Cache

```mermaid
flowchart LR
    SAM_Client["SAM Client"]
    SAM_Server["SAM server"]

    SAM_Client <--> SAM_Server

    subgraph SAM_Client
        direction LR
        CSP["Cached SAM Program"]
        CACHE["Cache"]
        CSP <--> CACHE
    end
```

