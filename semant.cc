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
static Stmt  curr_stmt = 0;
static Decl_class * to_main; //指向第一个main函数的指针，方便用于check main
static int  into_loop = 0;


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

///////////////////////////////////////////////////////////////////////////////////////////////
//
//需要实现一个类，用来存储函数名和参数类型
//使用此类时必须注意para数量不能大于6个，否则出错
///////////////////////////////////////////////////////////////////////////////////////////////

class func_paras{
    private:
        Symbol name;
        Symbol paras [6] ;//limite within 6 paras
        int num_paras ;
    
    public:
        func_paras(){num_paras = 0;for(int i = 0;i++;i<6)paras[i] = Void;}
        void setName(Symbol a){name = a ;/*cerr<<" make func-paras of "<<a<<endl;*/}
        void add_para(Symbol a){paras[num_paras++] = a;/*cerr<<" add paras "<<a<<" of "<<name<<endl;*/}
        Symbol getParaType(int num){ return paras[num];}
        Symbol getName(){return name;}
        int get_num_paras(){return num_paras;}
        ~func_paras(){}
};


///////////////////////////////////////////////////////////////////////////////////////////////

static func_paras functable[20];
static int num_func =0;
static void install_calls(Decls decls) {
    funcEnv.enterscope();
    funcEnv.addid(print, new Symbol(Void));//先保存下printf函数

    for(int i = decls->first(); decls->more(i); i = decls->next(i))
    {
        bool iscall = decls->nth(i)->isCallDecl();
        if (iscall){
            //bool isvalid = isValidCallName(decls->nth(i)->getType()); //函数名称不能是printf
            bool firstdecl = funcEnv.probe(decls->nth(i)->getName()) == NULL;
            bool return_exist = false;//还不能做返回值类型的检查，因为变量没有声明，也没有初始化！
                //check return exist
            CallDecl_class* curr_call;
            curr_call = reinterpret_cast<CallDecl_class *> (decls->nth(i));
                //check return exist
                for(int i = curr_call->getBody()->getStmts()->first(); 
                        curr_call->getBody()->getStmts()->more(i); 
                        i =curr_call->getBody()->getStmts()->next(i) 
                        )
                        {   if(curr_call->getBody()->getStmts()->nth(i)->judgeType() == 4){
                                return_exist = true;
                                //ReturnStmt_class * curr_stmt = reinterpret_cast<ReturnStmt_class *> (curr_call->getBody()->getStmts()->nth(i));
                                //Symbol returnType = curr_stmt->getValue()->getType();
                                //return_valid = (returnType == curr_call->getType() )? true: false;
                            }
                        }
                //if get paras ,save their type
                functable[num_func].setName(curr_call->getName());
                if (curr_call->getVariables()->len()>0 && curr_call->getVariables()->len()<7)
                {
                    for(int i = curr_call->getVariables()->first(); 
                        curr_call->getVariables()->more(i); 
                        i =curr_call->getVariables()->next(i) ){
                        functable[num_func].add_para( curr_call->getVariables()->nth(i)->getType() );
                        }
                    //cerr<<"save paras of "<<curr_call->getName()<<" succesfully"<<endl;
                    
                }
                else if (curr_call->getVariables()->len()>6)
                {
                    semant_error(decls->nth(i))<<"function "<<decls->nth(i)->getName()<<" get paras more than 6!"<<endl;break;
                }
                num_func ++;
                
            
            if( firstdecl && return_exist ){
                funcEnv.addid(decls->nth(i)->getName(), new Symbol( decls->nth(i)->getType()));
                if(decls->nth(i)->getName() == Main) curr_decl = decls->nth(i); //保存指向main的节点
            }
            //else if(!isvalid){
            //    semant_error(decls->nth(i))<<"function "<<decls->nth(i)->getName()<<" returnType invalid"<<endl;
            //}
            else if( !firstdecl)
            {
                semant_error(decls->nth(i))<<"redeclaration of func "<<decls->nth(i)->getName()<<endl;
            }
            else if (firstdecl && !return_exist)
            {
                semant_error(decls->nth(i))<<"return of func "<<decls->nth(i)->getName()<< " not exist in the overall scope"<<endl;
            }
        }
    }
    //cerr<<"打印idtable："<<endl;
    //idtable.print();
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
    //cerr<<"@ 5"<<endl;
    for(int i = decls->first(); decls->more(i) ; i = decls->next(i)){
        if( decls->nth(i)->isCallDecl()){
        curr_call =  reinterpret_cast<CallDecl_class *> (decls->nth(i));
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
        CallDecl_class * a = reinterpret_cast<CallDecl_class *> (curr_decl);
        //a->dump_with_types(cout ,0);
        bool no_variables =  a->getVariables()->len() == 0;
        if(!main_exist) semant_error(curr_decl)<<"main func do not exist"<<endl;
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
        bool is_not_void = isValidTypeName( paras->nth(i)->getType() );
        if(first_decl && is_not_void)
        objectEnv.addid( paras->nth(i)->getName() ,new Symbol(paras->nth(i)->getType()));
        else if (!first_decl ) {semant_error(paras->nth(i))<<"redeclaration of Var "<<paras->nth(i)->getName()<<" in the same scope "<<endl;}
        else if (first_decl && !is_not_void) {semant_error(paras->nth(i))<<"type  of Var "<<paras->nth(i)->getName()<<" can not be Void "<<endl;}
    }
    body->check(returnType);

    objectEnv.exitscope();
}

//传的参数应该是返回值类型，之前只是保证了最外环有一个合法的return ，
//但是要是检测内部其他的return，仍然需要知晓函数的返回类型
void StmtBlock_class::check(Symbol type) {
    //check vars
    objectEnv.enterscope();
    for(int i = vars->first(); vars->more(i); i = vars->next(i)){
        bool first_decl = objectEnv.probe(vars->nth(i)->getName()) == NULL;
        bool is_not_void = isValidTypeName( vars->nth(i)->getType() );
        if(first_decl && is_not_void)
            objectEnv.addid( vars->nth(i)->getName() ,new Symbol(vars->nth(i)->getType()));
        else if (!first_decl ) 
            {semant_error(vars->nth(i))<<"redeclaration of Var "<<vars->nth(i)->getName()<<" in the same scope "<<endl;}
        else if(first_decl && !is_not_void)
            {semant_error(vars->nth(i))<<"type  of Var "<<vars->nth(i)->getName()<<" can not be Void "<<endl;}
    }
    //check stmts
    //要特别注意continue，break的处理
    /*****************************************
    *增设一个判断句子类型的辅助函数judgeType()
    default 0
    IfStmt_class 1
    ForStmt_class 2
    WhileStmt_class 3
    ReturnStmt_class 4
    ContinueStmt_class 5
    BreakStmt_class 6
    Expr_class 7
    *
    *****************************************/
    for (int i = stmts->first(); stmts->more(i); i = stmts->next(i)){
        int type_of_stmt = stmts->nth(i)->judgeType();
        switch(type_of_stmt){
            case 1:{
                curr_stmt = stmts->nth(i);
                stmts->nth(i)->check(type);
            }break;

            case 2:{
                into_loop ++;
                curr_stmt = stmts->nth(i);//会有问题吗
                stmts->nth(i)->check(type);
                into_loop --;
            }break;

            case 3:{
                into_loop ++;
                curr_stmt = stmts->nth(i);
                stmts->nth(i)->check(type);
                into_loop --;
            }break;

            case 4:{
                curr_stmt = stmts->nth(i);
                stmts->nth(i)->check(type);
            }
            break;

            case 5:{
                curr_stmt = stmts->nth(i);
                stmts->nth(i)->check(type);
            }
            break;

            case 6:{
                curr_stmt = stmts->nth(i);
                stmts->nth(i)->check(type);
            }
            break;

            case 7:{
                curr_stmt = stmts->nth(i);
                stmts->nth(i)->check(type);
            }
            break;

            default:{semant_error(stmts->nth(i))<<"get an unknow wrong when analyze line "<<stmts->nth(i)->get_line_number()<<endl;}
        }

    }
}

void IfStmt_class::check(Symbol type) {
    bool condition_is_bool = condition->checkType() == Bool;
    if(!condition_is_bool) 
        {semant_error(curr_stmt)<<"condition of \"if\" ought to be Bool"<<endl;}
    thenexpr->check(type);
    elseexpr->check(type);
}

void WhileStmt_class::check(Symbol type) {
    bool condition_is_bool = condition->checkType() == Bool;
    if(!condition_is_bool) 
        {semant_error(curr_stmt)<<"condition of \"while\" ought to be Bool"<<endl;}
    body->check(type);

}

void ForStmt_class::check(Symbol type) {
    initexpr->checkType();
    loopact->checkType();
    bool condition_is_bool = condition->checkType() == Bool;
    bool condition_is_void = condition->is_empty_Expr();//要检查一下是不是Void 还是No_type
    if(condition_is_bool || condition_is_void) ;
    else{semant_error(curr_stmt)<<"condition of \"for\" ought to be Bool or Void"<<endl;}
    body->check(type);
}

void ReturnStmt_class::check(Symbol type) {
    Symbol a = value->checkType();
    if(type == a);
    else{semant_error(curr_stmt)<<"the return type is "<<a<<" but the function need "<<type<<endl;}   
}

void ContinueStmt_class::check(Symbol type) {
    if (into_loop > 0) ;
    else{semant_error(curr_stmt)<<"the continue sentence should be used in a loop"<<endl;}
}

void BreakStmt_class::check(Symbol type) {
    if (into_loop > 0) ;
    else{semant_error(curr_stmt)<<"the break sentence should be used in a loop"<<endl;}
}

Symbol Call_class::checkType(){
    bool exist = funcEnv.lookup(name) != NULL ;
    bool match = true;
    bool is_printf = name == (Symbol)print;
    if(!is_printf)
    {
        int i =0 ;
            for(; i<num_func;i++)
            {
                if(functable[i].getName() == name ) {
                    break;
                }
            }
        if(exist)
        {   
            if( functable[i].get_num_paras() == actuals->len() )
            {
                if(actuals->len() != 0)
                {
                    for(int j = actuals->first(); actuals->more(j); j = actuals->next(j)){
                        Symbol actualj = actuals->nth(j)->checkType();
                        Symbol fun_actualj =functable[i].getParaType(j);
                        if (fun_actualj != actualj) 
                        {
                            semant_error(curr_stmt)<<"the number "<<j<<" paras of call \""<<name<<"\" should be "<< fun_actualj<< " but we got "<<actualj<<endl;
                            match = false;
                        }
                        //else cerr<<"the number "<<j<<" paras of "<<name<<" match"<<endl;
                    }
                }
                setType(*(funcEnv.lookup(name)));//不管参数匹不匹配，函数已经存在，就应该返回声明的返回值。
                return type;
            }
            else 
            {
                semant_error(curr_stmt)<<"the number of actuals of the call is not match the func "<<name<<endl;
                setType(*(funcEnv.lookup(name)));//不管参数匹不匹配，函数已经存在，就应该返回声明的返回值。
                return type;
            }
        }
        else 
        {
            semant_error(curr_stmt)<<"can not find definition of "<<name<<endl;
            setType(Void);
            return type;
        }
    }
    else //is printf()
    {
        bool no_actuals = actuals->len() == 0 ;
        if (no_actuals) 
        {
            semant_error(curr_stmt)<<"the function printf must have one actuals at least"<<endl;
            setType(Void);
            return type;
        }
        else 
        { 
            int  i = actuals->first();
            Symbol first_actual = actuals->nth(i)->checkType();
            bool first_string = first_actual == String;
            if(!first_string)
            {
                semant_error(curr_stmt)<<"the first para of function printf ought to be String. but we got "<<first_actual<<endl;
                setType(Void);
                return type;
            }
        }
    }
    
}
//actuals 检查
Symbol Actual_class::checkType(){
    setType(expr->checkType());
    return type;
}

Symbol Assign_class::checkType(){
    Symbol l = *(objectEnv.lookup(lvalue)) ,r = value->checkType();
    if(objectEnv.lookup(lvalue) != NULL){
        //cerr<<"@ hello"<<endl;
        if (l != r){
            
            semant_error(curr_stmt)<<"the assign of "<<lvalue<<" from "<<r<<" to "<<l<<" not match"<<endl;
        }
        else{setType(*(objectEnv.lookup(lvalue)));
        return type;}
    }
    else{
        semant_error(curr_stmt)<<"can not find definition of "<<lvalue<<endl;
        setType(Void);
        return type;
        }
}

Symbol Add_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == a2 && (a1 == Int ||a1== Bool))
    {
        setType(a1);
        return type;
    }
    else if(a1 == Int && a2 == Float ||a1 == Float && a2 == Int)
    {
        setType(Float);
        return type;
    }
    else {
        semant_error(curr_stmt)<<"type of add expr between "<<a1<<" and "<<a2<<" not match or not valid"<<endl;
        setType(Void);
        return type;
        }
}

Symbol Minus_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == a2 && (a1 == Int ||a1== Bool))
    {
        setType(a1);
        return type;
    }
    else if(a1 == Int && a2 == Float ||a1 == Float && a2 == Int)
    {
        setType(Float);
        return type;
    }
    else {
        semant_error(curr_stmt)<<" type of minus expr between "<<a1<<" and "<<a2<<" not match or not valid"<<endl;
        setType(Void);
        return type;
        }
}

Symbol Multi_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == a2 && (a1 == Int ||a1== Bool))
    {
        setType(a1);
        return type;
    }
    else if(a1 == Int && a2 == Float ||a1 == Float && a2 == Int)
    {
        setType(Float);
        return type;
    }
    else {
        semant_error(curr_stmt)<<" type of multi expr between "<<a1<<"and"<<a2<<" not match or not valid"<<endl;
        setType(Void);
        return type;
        }
}

Symbol Divide_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == a2 && (a1 == Int ||a1== Bool))
    {
        setType(a1);
        return type;
    }
    else if(a1 == Int && a2 == Float ||a1 == Float && a2 == Int)
    {
        setType(Float);
        return type;
    }
    else {
        semant_error(curr_stmt)<<" type of divide expr between "<<a1<<" and "<<a2<<" not match or not valid"<<endl;
        setType(Void);
        return type;
        }
}

Symbol Mod_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == a2 && (a1 == Int ))
    {
        setType(a1);
        return type;
    }
    else {
        semant_error(curr_stmt)<<" type of mod expr between "<<a1<<" and "<<a2<<" not match or not valid"<<endl;
        setType(Void);
        return type;
        }
}

Symbol Neg_class::checkType(){
    Symbol a1 = e1->checkType();
    if((a1 == Int )||(a1 == Float ))
    {
        setType(a1);
        return type;
    }
    else {
        semant_error(curr_stmt)<<" type of neg expr to "<<a1<<" not valid"<<endl;
        setType(Void);
        return type;
        }
}

Symbol Lt_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if((a1 == Int ||a1 == Float) &&(a2 == Int || a2 == Float))
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not compare a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Le_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if((a1 == Int ||a1 == Float) &&(a2 == Int || a2 == Float))
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not compare a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Equ_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if((a1 == Int || a1== Float) && (a2 == Int || a2 == Float)||(a1 == Bool && a2 == Bool))
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not compare a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Neq_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if((a1 == Int || a1== Float) && (a2 == Int || a2 == Float)||(a1 == Bool && a2 == Bool))
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not compare a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Ge_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if((a1 == Int ||a1 == Float) &&(a2 == Int || a2 == Float))
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not compare a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Gt_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if((a1 == Int ||a1 == Float) &&(a2 == Int || a2 == Float))
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not compare a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol And_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == Bool && a2 == Bool)
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not make logic and between a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Or_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == Bool && a2 == Bool)
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not make logic or between a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Xor_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == Bool && a2 == Bool)
    {
        setType(Bool);
        return type;
    }
    else if(a1 == Int && a2 == Int)
    {
        setType(Int);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not make logic xor between a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Not_class::checkType(){
    Symbol a1 = e1->checkType();
    if(a1 == Bool )
    {
        setType(Bool);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not make logic not on a "<<a1<<endl;
        setType(Void);
        return type;
    }
}

Symbol Bitand_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == Int && a2 == Int)
    {
        setType(Int);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not make bit-and between a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Bitor_class::checkType(){
    Symbol a1 = e1->checkType(), a2 = e2->checkType();
    if(a1 == Int && a2 == Int)
    {
        setType(Int);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not make bit-or between a "<<a1<<" and a "<<a2<<endl;
        setType(Void);
        return type;
    }
}

Symbol Bitnot_class::checkType(){
    Symbol a1 = e1->checkType();
    if(a1 == Int)
    {
        setType(Int);
        return type;
    }
    else{
        semant_error(curr_stmt)<<"can not make bit-not on a "<<a1<<endl;
        setType(Void);
        return type;
    }
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
    if(objectEnv.lookup(var) != NULL)
    {
        setType(*objectEnv.lookup(var));
        return type;
    }
    else
    {
        semant_error(curr_stmt)<<"can not find definition of "<<var<<endl; 
        setType(Void);
        return type;
    }
}

////////////////////////////////////////////////////////
Symbol No_expr_class::checkType(){
    setType(Void);
    return getType();
}

void Program_class::semant() {
    initialize_constants(); //初始化常量类型
    install_calls(decls);   //初始化函数
    //funcEnv.dump();
    //objectEnv.dump();
    check_main();           //检查主函数
    install_globalVars(decls);//全局变量初始化
    check_calls(decls);       //检查函数合法性


    if (semant_errors > 0) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}



