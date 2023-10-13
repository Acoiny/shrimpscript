# shrimpp
My first attempt at creating my own interpreted language. It still borrows a lot of stuff from "Crafting Interpreters" by Robert Nystrom,
though I added a lot of stuff myself.

## single line comments
```
//this is a comment
```
## variables:
```
let a = "i am a changeable variable";  
const b = "i can't be changed";  
```
**NOTE: const variables currently don't work in repl, due to them being compile-time evaluated**
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
for(let i = 0; i < 10; i = i + 1) {  
  //do something  
  if(condition)  
    break;  
}  
```
### while-loop
```
while(condition) {  
  //do another thing  
}  
```
## functions:
```
fun foo(num) {  
  //do stuff  
  return result;  
}
```
**NOTE: functions are first class objects, but cannot be declared in blocks/other functions - also closures aren't supported (yet?)**
## import
```
import 'file.shrimp';
```
**import takes a file path (relative to the current directory) and executes the corresponding file
this can be used to import functions/classes and more**
## classes:
### class declaration
```
class bar{  
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
class foo : bar{
  init() {
    //super keyword calls superclass function
    super.init('foo');
    //can also access superclass variables (only class variables)
    super.variable = 32;
  }
}
```
### creating class instance
```
let instance = bar("jeff");
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
**dictionaries allow the storing two value pairs, with the first being used as the key to access the second**
