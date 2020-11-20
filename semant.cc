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
static Decl_class * to_main; //指向第一个main函数的指针，方便用于check main

typedef SymbolTable<Symbol, Symbol> ObjectEnvironment; // name, type  
ObjectEnvironment objectEnv,funcEnv; //函数表，变量表
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
    funcEnv.enterscope();
    funcEnv.addid(print, new Symbol(Void));
    for(int i = decls->first(); decls->more(i); i = decls->next(i))
    {
        bool iscall = decls->nth(i)->isCallDecl();
        if (iscall){
            bool isvalid = isValidCallName(decls->nth(i)->getType());
            bool firstdecl = funcEnv.probe(decls->nth(i)->getName()) == NULL;
            bool return_exist = false;
            bool return_valid = false;
                {
                    CallDecl_class* curr_call;
                    curr_call = reinterpret_cast<CallDecl_class *> (decls->nth(i));
                    for(int i = curr_call->getBody()->getStmts()->first(); 
                        curr_call->getBody()->getStmts()->more(i); 
                        i =curr_call->getBody()->getStmts()->next(i) 
                        )
                        {   if(curr_call->getBody()->getStmts()->nth(i)->judgeType() == 4){
                                return_exist = true;
                                ReturnStmt_class * curr_stmt = reinterpret_cast<ReturnStmt_class *> (curr_call->getBody()->getStmts()->nth(i));
                                Symbol returnType = curr_stmt->getValue()->getType();
                                return_valid = (returnType == curr_call->getType() )? true: false;
                        }

                        }
                }
            if(isvalid && firstdecl && return_exist && return_valid){
                funcEnv.addid(decls->nth(i)->getName(), new Symbol( decls->nth(i)->getType()));
                if(decls->nth(i)->getName() == Main) curr_decl = decls->nth(i); //保存指向main的节点
            }
            else if(!isvalid){
                semant_error(decls->nth(i))<<"function "<<decls->nth(i)->getName()<<" returnType invalid"<<endl;
            }
            else if( isvalid && !firstdecl)
            {
                semant_error(decls->nth(i))<<"redeclaration of func "<<decls->nth(i)->getName()<<endl;
            }
            else if (isvalid && firstdecl && !return_exist)
            {
                semant_error(decls->nth(i))<<"return of func "<<decls->nth(i)->getName()<< " not exist in the overall scope"<<endl;
            }
            else if (isvalid && firstdecl && return_exist && !return_valid)
            {
                semant_error(decls->nth(i))<<"return of func "<<decls->nth(i)->getName()<< " not match"<<endl;
            }
        }
    }
    cerr<<"打印idtable："<<endl;
    idtable.print();
    //objectEnv.exitscope();

}

static void install_globalVars(Decls decls) {

    objectEnv.enterscope(); //需要与其他函数联动考量是否打开一个scope
    for(int i = decls->first(); decls->more(i); i = decls->next(i)){
        bool isvardecl = !decls->nth(i)->isCallDecl();
        bool isvalid = isValidTypeName(decls->nth(i)->getType());
        bool firstdecl = objectEnv.probe(decls->nth(i)->getName()) == NULL;
        
         if( isvardecl
         && firstdecl
         && isvalid
          ){
            objectEnv.addid(decls->nth(i)->getName(), new Symbol( decls->nth(i)->getType()));
         }
         else if(isvardecl
         && !isvalid
         ){
              semant_error(decls->nth(i))<<"VAR declaration type invalid, can not be void. "<<endl;
         }
         else if(isvardecl
         && isvalid
         && !firstdecl
         ){
              semant_error(decls->nth(i))<<"redeclaration of var "<<decls->nth(i)->getName()<<endl;
         }
    }
        
    //objectEnv.exitscope();
}

static void check_calls(Decls decls) {
    //check function calls one by one
    //int to_main;
    CallDecl_class* curr_call;
    cerr<<"@ 5"<<endl;
    for(int i = decls->first(); decls->more(i) ; i = decls->next(i)){
        if( decls->nth(i)->isCallDecl()){
        curr_call = reinterpret_cast<CallDecl_class *> (decls->nth(i));
        //cerr<<"@ 3"<<endl;
        curr_call->check();
        //cerr<<"@ 4"<<endl;
        }
    }
}

static void check_main() {
    
    bool main_exist = funcEnv.probe(Main) != NULL ;
    if (main_exist){
    bool return_void = *(funcEnv.probe(Main)) == Void ; 
    cerr<<"@段错误@"<<endl;
    CallDecl_class * a = reinterpret_cast<CallDecl_class *> (curr_decl);
    //a->dump_with_types(cout ,0);
    bool no_variables =  a->getVariables()->len() == 0;
    if(!main_exist) semant_error()<<"main func do not exist"<<endl;
    else if (main_exist && !return_void){
        semant_error(curr_decl)<<"main func ought to return Void"<<endl;
    }
    else if (main_exist && return_void && !no_variables){
        semant_error(curr_decl)<<"main func ought to have no variables"<<endl;
    }
    }
    //objectEnv.exitscope(); //函数名的检查处理结束
}

void VariableDecl_class::check() {
    //idtable.lookup_string((char *)(this->getName()));
}

void CallDecl_class::check() {
    //把形参加入作用域并检查形参是否重复,是否合法
    objectEnv.enterscope();
    for(int i = paras->first(); paras->more(i); i = paras->next(i)){
        bool first_decl = objectEnv.probe(paras->nth(i)->getName()) == NULL;
        bool is_not_void = paras->nth(i)->getType() != Void;
        if(first_decl && is_not_void)
        objectEnv.addid( paras->nth(i)->getName() ,new Symbol(paras->nth(i)->getType()));
        else if (!first_decl ) {semant_error(paras->nth(i))<<"redeclaration of Var "<<paras->nth(i)->getName()<<" in the same scope "<<endl;}
        else if (first_decl && !is_not_void) {semant_error(paras->nth(i))<<"type  of Var "<<paras->nth(i)->getName()<<" can not be Void "<<endl;}
    }

    objectEnv.exitscope();
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
        funcEnv.dump();
    objectEnv.dump();
    check_main();           //检查主函数
    install_globalVars(decls);//全局变量初始化
    check_calls(decls);       //检查函数合法性


    if (semant_errors > 0) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}



