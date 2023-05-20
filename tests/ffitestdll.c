
typedef int (*adder2)(int a,int b ,void *p);

extern int add(int a,int b,adder2 adder){
    if(adder==0){
        return a+b;
    }else{
        return adder(a,b,0);
    }
}