# shrimpscript <img src='/ressource/shrimp.ico' width='25'>
My first attempt at creating my own interpreted language. It still borrows a lot of stuff from "Crafting Interpreters" by Robert Nystrom,
though I added a lot of stuff myself.

## single line comments
```
// this is a comment
```
## ASI (automatic semicolon insertion)
The parser automatically inserts semicolons, if a illegal Token is found, but preceded by a new line or if the illegal Token is '}' or the end of the file it inserts a semicolon before it
```
let a = 2
a = 1

return
a
+ 2
```
becomes
```
let a = 2;
a = 1;

return a + 2;
```
**This feature is still in testing and may lead to valid, but unintended code!**
## variables:
```
let a = "i am a changeable variable";
const b = "i can't be changed";

//multiple declarations
let i = 1, j = 1;
const n = 5, m = 10;
```
### strings
```
let str1 = "hello\tworld";  //contains "hello  world"
let str2 = 'hello\tworld';  //contains 'hello\tworld'
```
**double quoted strings allow basic escape sequences, single quote strings are taken as is**
## control flow similar to C
### if-statement
```
if(condition) {  
  //do a  
} else {  
  //do b  
}  
```
### for-loop
```
for(let i = 0; i < 10; i++) {  
  //do something  
  if(condition)  
    break;  //break exits current loop
}  
```
### while-loop
```
while(condition) {  
  //do another thing
  if(condition)
    continue;  //continue jumps to start of current loop  
}  
```
## functions:
```
fun foo(num) {  
  //do stuff  
  return result;  
}
```
Functions can also be declared inside other functions and may capture local variables
```
fun foo(num) {
  fun bar(other) {
    return num + other;
  }

  return bar;
}
```
## import
```
import "file.shrimp";
```
**import takes a file path (relative to the executed file) and executes the corresponding file
this can be used to import functions/classes and more**
**BUG: file might not be recognized if using single quotes for name**
## classes:
### class declaration
```
class Bar{  
  variable = 23;  

  //init function gets called when creating objects (optional)
  fun init(name) {  
    this.name = name;  
  }  
  doBar() {
    //code
  }
}  
```
### inheritance
```
class Foo : Bar{
  init() {
    //super keyword calls superclass function
    super.init('foo');
  }
}
```
### creating class instance
```
let instance = Bar("jeff");
```
### getting and setting instance properties
```
let name = instance.name;
instance.name = "not jeff anymore";  
```
## lists
```
const list = ["a", 23, bar];  
list.append(23);  
list[0] = "b";
const length = list.len();
```
## dictionaries
```
const dict = {"boss": "jeffrey", "employee": "bob", 0: "number"};
let boss = dict["boss"]; //boss = "jeffrey"
let empty = dict['hello']; //empty = nil
dict['new'] = 42; //sets new field
```
**dictionaries allow the storing of value pairs, with the first being used as the key to access the second**

# Flags when compiling
## NAN_BOXING
When set (default) the values are represented by a union
```
union {
  uint64 as_uint64;
  double as_double;
} value;
```
Doubles are read as is, all other values are encoded in the lower bits of NaN values
This serves the purpose of only taking up 8 bytes instead of 9 (16 with padding) and improves performance this way. If the flag is not set, value will be represented as a tagged union:
```
class value {
  valType type;

  union {
    bool boolean;
    obj* object;
    double number;
  }
  //...
}
```
It is recommended to remove the flag when using 32-bit architecture
## debug flags in "defines.hpp"
### DEBUG_TESTFILE <FILENAME>
If set, the compiler always compiles <FILENAME> when being executed without arguments

### DEBUG_PRINT_CODE
The bytecode is disassembled and printed to the screen after compiling

### DEBUG_TRACE_EXECUTION
The current instruction and the current stack are disassembled and printed to the screen

### DEBUG_STRESS_GC
The garbage collector is called everytime an object is created

### DEBUG_LOG_GC
The actions of the garbage collector are printed to the screen
