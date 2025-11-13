// On deref, object ends up in cache
// further derefs will first check the cache

// pointer alive to object on client increments refcount
// when refcount on host = 0, object dropped or evicted to server

// how to know when to auto-drop?
// if pointers on server reach 0 too
// destroy() operation on server actually drops it from the client

// if client OOM, then evict to server

// 

// pointers falling out of scope will 