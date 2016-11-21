# A proposal to add non-owning indirection class templates `indirect` and `optional_indirect`

_Joseph Thomson \<joseph.thomson@gmail.com\>_

## Introduction

## Table of contents

## Motivation and scope

## Impact on the standard

## Design decisions

### Construction from `T&`

### Conversion to `T&`

### Construction from `T*`

### Conversion to `T*`

### Construction from `T&&`

### Assignment operators and the `{}` idiom

### Supplementary optional accessors (`value` and `value_or`)

### Comparison operations

### Move behaviour

### Relationships and immutability

### Const-propagation

### The `make` free functions

### The `indirect_cast` free functions

### The `get_pointer` free functions

### Compatibility with `propagate_const`

### Use of inheritance by delegation ("`operator.` overloading")

### Why not `T*`?

### Why not `T&`?

### Why not `optional<T&>`?

### Why not `optional<indirect<T>>`?

### Why not `observer_ptr<T>`?

### Why not `reference_wrapper<T>`?

### Why not `not_null<T*>`?

### Naming

## Technical specifications

## Acknowledgements

## References
