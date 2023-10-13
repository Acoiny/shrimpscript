# shrimpp
My first attempt at creating my own interpreted language. It still borrows a lot of stuff from "Crafting Interpreters" by Robert Nystrom,
though i tried my best to make it a bit original.

# single line comments
```
//this is a comment
```
# variables:
```
let a = "i am a variable";  
const b = "i can't be changed";  
```
# control flow similar to C
```
if(condition) {  
  //do a  
} else {  
  //do b  
}  
```
```
for(let i = 0; i < 10; i = i + 1) {  
  //do something  
  if(condition)  
    break;  
}  
```
```
while(condition) {  
  //do another thing  
}  
```
# funtions:
```
fun foo(num) {  
  //do stuff  
  return result;  
}
```
(NOTE: functions are first class objects, but cannot be declared in blocks/other functions - also closures aren't supported (yet?))  

# classes:
```
class bar{  
  variable = 23;  

  //init function gets called when creating objects (optional)  
  fun init(name) {  
    this.name = name;  
  }  

}  

let instance = bar("jeff");  
bar.name = "not jeff anymore";  
```
# lists
```
const list = ["a", 23, bar];  
list.append(23);  
list[0] = "b";  
```
# dictionaries
```
const dict = {"boss": "jeffrey", "employee": "bob"};  //keys MUST be strings
```
