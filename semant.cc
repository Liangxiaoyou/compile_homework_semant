#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"

extern int semant_debug;
extern char *curr_filename;

static ostream& error_stream = cerr; //输出错误信息流
static int semant_errors = 0; //错误计数
static Decl curr_decl = 0;
static tree_node * to_main; //指向第一个main函数的指针，方便用于check main

typedef SymbolTable<Symbol, Symbol> ObjectEnvironment; // name, type  
ObjectEnvironment objectEnv; //一个保存symbol的列表

///////////////////////////////////////////////
// helper func
///////////////////////////////////////////////


static ostream& semant_error() {
    semant_errors++;
    return error_stream;
}

static ostream& semant_error(tree_node *t) {
    error_stream << t->get_line_number() << ": ";
    return semant_error();
}

static ostream& internal_error(int lineno) {
    error_stream << "FATAL:" << lineno << ": ";
    return error_stream;
}

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////

static Symbol 
    Int,
    Float,
    String,
    Bool,
    Void,
    Main,
    print
    ;

bool isValidCallName(Symbol type) {
    return type != (Symbol)print;
}

bool isValidTypeName(Symbol type) {
    return type != Void;
}

//
// Initializing the predefined symbols.
//

static void initialize_constants(void) {
    // 4 basic types and Void type
    Bool        = idtable.add_string("Bool");
    Int         = idtable.add_string("Int");
    String      = idtable.add_string("String");
    Float       = idtable.add_string("Float");
    Void        = idtable.add_string("Void");  
    // Main function
    Main        = idtable.add_string("main");

    // classical function to print things, so defined here for call.
    print       = idtable.add_string("printf");
}

/*
    TODO :
    you should fill the following function defines, so that semant() can realize a semantic 
    analysis in a recursive way. 
    Of course, you can add any other functions to help.
*/

static bool sameType(Symbol name1, Symbol name2) {
    return strcmp(name1->get_string(), name2->get_string()) == 0;
}

static void install_calls(Decls decls) {
    objectEnv.enterscope();
    for(int i = decls->first(); decls->more(i); i = decls->next(i))
    {
        //cout<<" "<<decls->nth(i)->isCallDecl()<<endl;
       if(  decls->nth(i)->isCallDecl() 
            && isValidCallName(decls->nth(i)->getType())
            && objectEnv.probe(decls->nth(i)->getName()) == NULL 
          ){
           objectEnv.addid(decls->nth(i)->getName(), new Symbol( decls->nth(i)->getType()));
           if(decls->nth(i)->getName() == Main) to_main = decls->nth(i); //保存指向main的节点。
       }
        else if(decls->nth(i)->isCallDecl() 
            && !isValidCallName(decls->nth(i)->getType())
        ){
              //internal_error(decls->nth(i)->get_line_number());
              semant_error(decls->nth(i))<<"function returnType invalid"<<endl;
              //cerr<< "function returnType invalid"<<endl;
        }
        else if(decls->nth(i)->isCallDecl() 
                &&  isValidCallName(decls->nth(i)->getType())
                &&  objectEnv.probe(decls->nth(i)->getName()) != NULL
                    )
          {
              //internal_error(decls->nth(i)->get_line_number());
              semant_error(decls->nth(i))<<"conflict function name"<<endl;
              //cerr<< "conflict function name"<<endl;
          }

    }
    //objectEnv.exitscope();

}

static void install_globalVars(Decls decls) {
    objectEnv.enterscope();
    for(int i = decls->first(); decls->more(i); i = decls->next(i)){
         if( !decls->nth(i)->isCallDecl()
         && objectEnv.probe(decls->nth(i)->getName()) == NULL 
         && isValidTypeName(decls->nth(i)->getType())
          ){
            objectEnv.addid(decls->nth(i)->getName(), new Symbol( decls->nth(i)->getType()));
         }
         else if(!decls->nth(i)->isCallDecl()
         && !isValidTypeName(decls->nth(i)->getType())
         ){
              
              semant_error(decls->nth(i))<<"VAR declaration type invalid"<<endl;

         }
         else if(!decls->nth(i)->isCallDecl()
         && isValidTypeName(decls->nth(i)->getType())
         && objectEnv.probe(decls->nth(i)->getName()) != NULL 
         ){

              semant_error(decls->nth(i))<<"redeclaration of var "<<decls->nth(i)->getName()<<endl;

         }
    }
    //objectEnv.exitscope();
}

static void check_calls(Decls decls) {

}

static void check_main() {
    if(objectEnv.probe(Main) == NULL) semant_error()<<"main func do not exist"<<endl;
    else if (objectEnv.probe(Main) != NULL && *(objectEnv.lookup(Main)) != Void){
        semant_error(to_main)<<"main func ought to return Void"<<endl;
    }
    objectEnv.exitscope();
}

void VariableDecl_class::check() {
    //idtable.lookup_string((char *)(this->getName()));
}

void CallDecl_class::check() {

}

void StmtBlock_class::check(Symbol type) {

}

void IfStmt_class::check(Symbol type) {

}

void WhileStmt_class::check(Symbol type) {

}

void ForStmt_class::check(Symbol type) {

}

void ReturnStmt_class::check(Symbol type) {

}

void ContinueStmt_class::check(Symbol type) {

}

void BreakStmt_class::check(Symbol type) {

}

Symbol Call_class::checkType(){
    setType(Float);
    return type;
}

Symbol Actual_class::checkType(){
setType(Float);
    return type;
}

Symbol Assign_class::checkType(){
setType(Float);
    return type;
}

Symbol Add_class::checkType(){
setType(Float);
    return type;
}

Symbol Minus_class::checkType(){
setType(Float);
    return type;
}

Symbol Multi_class::checkType(){
setType(Float);
    return type;
}

Symbol Divide_class::checkType(){
setType(Float);
    return type;
}

Symbol Mod_class::checkType(){
setType(Float);
    return type;
}

Symbol Neg_class::checkType(){
setType(Float);
    return type;
}

Symbol Lt_class::checkType(){
setType(Float);
    return type;
}

Symbol Le_class::checkType(){
setType(Float);
    return type;
}

Symbol Equ_class::checkType(){
setType(Float);
    return type;
}

Symbol Neq_class::checkType(){
setType(Float);
    return type;
}

Symbol Ge_class::checkType(){
setType(Float);
    return type;
}

Symbol Gt_class::checkType(){
setType(Float);
    return type;
}

Symbol And_class::checkType(){
setType(Float);
    return type;
}

Symbol Or_class::checkType(){
setType(Float);
    return type;
}

Symbol Xor_class::checkType(){
setType(Float);
    return type;
}

Symbol Not_class::checkType(){
setType(Float);
    return type;
}

Symbol Bitand_class::checkType(){
setType(Float);
    return type;
}

Symbol Bitor_class::checkType(){
setType(Float);
    return type;
}

Symbol Bitnot_class::checkType(){
setType(Float);
    return type;
}
////////////////////////////////////////////////////////////
Symbol Const_int_class::checkType(){
    setType(Int);
    return type;
}

Symbol Const_string_class::checkType(){
    setType(String);
    return type;
}

Symbol Const_float_class::checkType(){
    setType(Float);
    return type;
}

Symbol Const_bool_class::checkType(){
    setType(Bool);
    return type;
}
////////////////////////////////////////////////////////
Symbol Object_class::checkType(){
    setType(getType());
    return type;
}

////////////////////////////////////////////////////////
Symbol No_expr_class::checkType(){
    setType(Void);
    return getType();
}

void Program_class::semant() {
    initialize_constants(); //初始化常量类型
    install_calls(decls);   //初始化函数
    check_main();           //检查主函数
    install_globalVars(decls);//全局变量初始化
    check_calls(decls);       //检查函数合法性
    
    objectEnv.dump();

    if (semant_errors > 0) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}



