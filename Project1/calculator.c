#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <ctype.h>

#define OPERAND_MAX_SIZE 200            // max supported size for single operand
#define INUPUT_MAX_SIZE 512             // max supported size for input
#define MAX_E 30                        // the scale threshold for normal print and scientific notation 
#define ANS_PRECISION 5                 // the precision of answer in scientific notation 
#define BASE_PRECISION 40               // the basic length of answer in divide operation
#define PI "3.1415926535897932384626"   // the value of PI
#define TAYLOR_EXPANSION_MAX_ORDER 20   // the max order or taylor expansion in sin and cos funtions
#define SIN_PRECISON 8                  // the precision of sin produced answer

/* The functions that are used to parse the inputs into
 * valid parameters that can be used for calculations
 */
int parse_number(const char * input, int i, char * para,int * symbol);
int parse_func(const char * input, int *func_code, char * para,int * symbol);
int parse_expression(const char * input, char *num1, char *op, char *num2,int * num1_symbol,int * num2_symbol);
int parse_and_calculate(const char * input, char *num1, char *op, char *num2,int * num1_symbol,int * num2_symbol,int type);

/* The functions that the calculator supported */
char * add(const char * a,const char * b);
char * subtract(const char * a,const char * b, int type);
char * multiply(const char * a,const char * b);
char * divide(const char * a,const char * b);
char * square_root(const char * a);
char * sine(const char * a, int * minus_flag);
char * cosine(const char * a,int * is_minus);

/* Some inner used functions */
double max(double a , double b);
double min(double a , double b);
char * trans_to_ans(int ans_Digits[],int dig_len,int decimal_pos);
char * tens_power_of(int power);

/* to output the answer in scientific notation */
int scientific_notation(char * answer, int ans_len, unsigned int precision,int max_e);


int main(int argc, char * argv[]) 
{
    /* initialize our most used variables */
    char * num1 = malloc(OPERAND_MAX_SIZE * sizeof(char));
    char * num2 = malloc(OPERAND_MAX_SIZE * sizeof(char));
    int * num1_symbol = malloc(sizeof(int));
    int * num2_symbol = malloc(sizeof(int));
    char operator[5] = {0};
    char * func = malloc(20 * sizeof(char));
    char input[INUPUT_MAX_SIZE];

    switch(argc)
    {
        /* case 1: no parameter input,go into a loop until we input quit or exit */
        case 1:
            while (1) 
            {
                printf(">>> ");

                /* preprocess the input, to make it valid*/
                if (fgets(input, sizeof(input), stdin) == NULL)
                    return 0;
                
                input[strcspn(input, "\n")] = '\0';

                char * start = input;
                while (*start && isspace((unsigned char)*start))
                    start++;

                if (start != input)
                    memmove(input, start, strlen(start) + 1);

                /* if input quit or exit, exit the program */
                if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0)
                    return 0;

                /* a simple help document, shows the supported functions and their valid input forms */
                if (strcmp(input, "help") == 0) 
                {
                    printf("This calculator supports operations below:\n");
                    printf("add\t\t <symbol>[number1] + <symbol>[number2]\n");
                    printf("subtract\t <symbol>[number1] - <symbol>[number2]\n");
                    printf("multiply\t <symbol>[number1] *(or x) <symbol>[number2]\n");
                    printf("divide\t\t <symbol>[number1] / <symbol>[number2]\n");
                    printf("squre root\t sqrt([number1])\n");
                    printf("sin\t\t sin(<symbol>[number1])\n");
                    return 0;
                }

                /* by detecting the first char, decide whether it is a expression or a function */
                int is_num = 0;
                if(isdigit(input[0]) || input[0]=='-'||input[0]=='+')
                    is_num = 1;
                
                /* is an expression */
                if(is_num)
                {
                    int output_success = parse_and_calculate(input, num1, operator, num2,num1_symbol,num2_symbol,0);
                    if(!output_success)
                        printf("failed output\n");
                }
                /* is a function */
                else
                {
                    func = input;
                    int * symbol = malloc(sizeof(int));
                    int * is_minus = malloc(sizeof(int));
                    char * para = malloc(OPERAND_MAX_SIZE * sizeof(char));
                    char * result = malloc(2*(strlen(para)) * sizeof(char));
                    int func_code = 0;
                    int note = parse_func(func, &func_code, para, symbol);
                    if (note!=2) 
                    {
                        printf("not an valid function\n");
                        free(symbol);
                        free(para);
                        free(result);
                        return 0;
                    }

                    switch (func_code) 
                    {
                        case 1:
                            if(*symbol)
                            {
                                printf("The parameter of sqrt cannot be a minus number\n");
                                return 0;
                            }
                            result = square_root(para);
                            scientific_notation(result,strlen(result),ANS_PRECISION,MAX_E);
                            printf("%s\n",result);
                        break;
                        case 2:
                            result = sine(para,is_minus);
                            result[SIN_PRECISON+2] ='\0'; 
                            scientific_notation(result,strlen(result),ANS_PRECISION,MAX_E);
                            if(*symbol && *is_minus)
                                printf("%s\n",result);
                            else if(*symbol && !*is_minus)
                                printf("-%s\n",result);
                            else if(!*symbol && *is_minus)
                                printf("-%s\n",result);
                            else
                                printf("%s\n",result);
                        break;
                        // case 3:
                        //     result = cosine(para,is_minus);
                        //     scientific_notation(result,strlen(result),ANS_PRECISION,MAX_E);
                        //     if(*is_minus)
                        //         printf("-%s\n",result);
                        //     else
                        //         printf("%s\n",result);
                        // break;
                        default:
                            printf("unsupported function code: %d\n", func_code);
                            break;
                    }
                    free(is_minus);
                    free(symbol);
                    free(result);
                    free(para);
                }
            }
        break;

        /* according to different number of input parameters, 
         * concatencate it into string input again 
         */
        case 2:
            strcat(input,argv[1]);
        break;
        case 3:
            strcat(input,argv[1]);
            strcat(input,argv[2]);
        break;
        case 4:
            strcat(input,argv[1]);
            strcat(input,argv[2]);
            strcat(input,argv[3]);
        break;
        default:
            printf("incorrect number of parameter:%d\n",argc);
            return 0;

    }


    /* the same as case 1 */
    int is_num = 0;
    if(isdigit(input[0]) || input[0]=='-'||input[0]=='+')
        is_num = 1;
    
    if(is_num)
    {
        int output_success = parse_and_calculate(input, num1, operator, num2,num1_symbol,num2_symbol,1);
        if(!output_success)
            printf("failed output\n");
    }else{
        func = input;
        int * symbol = malloc(sizeof(int));
        int * is_minus = malloc(sizeof(int));
        char * para = malloc(OPERAND_MAX_SIZE * sizeof(char));
        char * result = malloc(2*(strlen(para)) * sizeof(char));
        int func_code = 0;
        int note = parse_func(func, &func_code, para, symbol);
        /* note = 0: neither func_name nor para is valid
         * note = 1: only one of func_name and para is valid
         * note = 2: both func_name and para are valid
         */
        if (note != 2) 
        {
            printf("not an valid function\n");
            return 0;
        }
        //scientific_notation(para,strlen(para),ANS_PRECISION,MAX_E);
        switch (func_code) 
        {
            case 1:
                if(*symbol)
                {
                    printf("The parameter of sqrt cannot be a minus number\n");
                    return 0;
                }
                result = square_root(para);
                scientific_notation(result,strlen(result),ANS_PRECISION,MAX_E);
                printf("sqrt(%s) = %s\n", para, result);
            break;
            case 2:
                result = sine(para,is_minus);
                result[SIN_PRECISON+2] ='\0'; 
                scientific_notation(result,strlen(result),ANS_PRECISION,MAX_E);
                if(*symbol && *is_minus)
                    printf("sin(-%s) = %s\n",para,result);
                else if(*symbol && !*is_minus)
                    printf("sin(-%s) = -%s\n",para,result);
                else if(!*symbol && *is_minus)
                    printf("sin(%s) = -%s\n",para,result);
                else
                    printf("sin(%s) = %s\n",para,result);
            break;
            // case 3:
            //     result = cosine(para,is_minus);
            //     scientific_notation(result,strlen(result),ANS_PRECISION,MAX_E);
            //     //printf("%s\n",result);
            //     if(*symbol && *is_minus)
            //         printf("cos(-%s) = -%s\n",para,result);
            //     else if(*symbol && !*is_minus)
            //         printf("cos(-%s) = %s\n",para,result);
            //     else if(!*symbol && *is_minus)
            //         printf("cos(%s) = -%s\n",para,result);
            //     else
            //         printf("cos(%s) = %s\n",para,result);
            // break;
            default:
                printf("unsupported function code: %d\n", func_code);
            break;
        }
    }
    return 1;
}

/* parse one string into a valid number, used by all parse_* functions */
int 
parse_number(const char * input, int i, char * para, int * symbol)
{

    /* check symbol for the number */
    if(input[i] == '-')
    {
        *symbol = 1;
        i++;
    }
    else if(input[i] == '+')
    {
        *symbol = 0;
        i++;
    }
    else if(isdigit(input[i]))
        *symbol = 0;
    else
    {
        printf("number symbol is invalid, ");
        return 0;
    }

    /* parameter index */
    int j = 0;
    /* only one dot and e symbol for a number */
    int num_dot_flag = 0;
    int num_e_flag = 0;

    /* parse the parameter */
    while (i < strlen(input) && input[i] != ')' && (isdigit(input[i]) || input[i] == '.' || input[i] == 'e' || input[i] == '-' || input[i] == '+')) 
    {
        if(input[i] == '.')
        {
            if(num_e_flag){
                printf("no decimal point after e notaion, ");
                return 0;
            }
            else
                num_dot_flag++;
        }

        if(input[i] == 'e')
            num_e_flag++;

        if(num_dot_flag<2 && num_e_flag<2)
        {
            if(input[i] == '-' || input[i] == '+')
            {
                if(input[i-1]=='e')
                    para[j++] = input[i++];
                else
                {
                    if(strlen(para)>=1)
                        break;
                    else
                        return 0;
                }
            }
            else
                para[j++] = input[i++];
        }
        else
        {
            printf("invalid input of the number, ");
            return 0;
        }
    }
    if(para[j-1]=='e' || para[j-1]=='.' || para[j-1]=='-' || para[j-1]=='+')
    {
        printf("number not specified, ");
        return 0;
    }
    para[j] = '\0';
    j = 0;

    /* translate numbers with e symbol into normal representation */
    if(num_e_flag)
    {
        char * tempt_num = malloc(strlen(para) * sizeof(char));
        char * power = malloc(strlen(para) * sizeof(char));

        int idx = 0;
        int t_idx = 0;

        while(para[idx]!= 'e')
            tempt_num[t_idx++] = para[idx++];
        tempt_num[t_idx]='\0';

        idx++;
        t_idx = 0;

        while(para[idx]!= '\0')
            power[t_idx++] = para[idx++];
        power[t_idx]='\0';

        strcpy(para,multiply(tempt_num,tens_power_of(atof(power))));

        free(tempt_num);
        free(power);
    }

    return i;
}

/* parse input string into function name and its parameter */
int 
parse_func(const char * input, int * func_code, char * para,int * symbol) 
{

    char func_name[50] = {0};
    int i = 0;int j = 0;

    /* jump prefix spaces */
    while (i < strlen(input) && isspace(input[i]))
        i++;

    /* make the characters before left parentheses to be the function name */
    while(i < strlen(input) && input[i] != '(' && input[i] != '-' && input[i] != '+')
        func_name[j++] = input[i++];
    
    func_name[j] = '\0';
    i++;

    if(i > strlen(input))
    {
        printf("wrong format for the function, ");
        return 0;
    }

    /* parse the parameter */
    i = parse_number(input, i, para, symbol);
    if(!i)
    {
        printf("parse number fail, ");
        return 0;
    }

    /* check if all valid */
    if(strlen(para)>0)
    {
        if (strcmp(func_name, "sqrt") == 0) 
        {
            *func_code = 1;
            return 2;
        }
        else if(strcmp(func_name, "sin") == 0) 
        {
            *func_code = 2;
            return 2;
        }
        // else if(strcmp(func_name, "cos") == 0) 
        // {
        //     *func_code = 3;
        //     return 2;
        // }
        else
        {
            printf("wrong input for the name\n");
            return 1;
        }
    }
    return 0;
}

/* parse input string into three valid parts: operand1, operator, operand2 */
int 
parse_expression(const char * input, char * num1, char * op, char * num2,int * num1_symbol, int * num2_symbol) 
{
    int i = 0, j = 0;

    /* jump prefix space */
    while (i < strlen(input) && isspace(input[i]))
        i++;

    
    /* parse the first number */
    i = parse_number(input, i, num1, num1_symbol);
    if(!i)
    {
        printf("parse number fail, ");
        return 0;
    }

    /* jump inner space */
    while (i < strlen(input) && isspace(input[i]))
        i++;

    /* parse the operator */
    if(!isdigit(input[i]) && !isspace(input[i]) && input[i] != '\0')
        op[0] = input[i++];

    if(op[0]==0)
    {
        printf("no operator, ");
        return 0;
    }
    op[1] = '\0';

    /* jump inner space */
    while (i < strlen(input) && isspace(input[i]))
        i++;


    /* parse the second number */
    i = parse_number(input, i, num2, num2_symbol);
    if(!i)
    {
        printf("parse number fail, ");
        return 0;
    }

    /* check the postfix */
    while(i < strlen(input) && input[i]!=0)
    {
        while (i < strlen(input) && isspace(input[i]))
            i++;

        if(i < strlen(input) && input[i] != 0)
        {
            printf("undeclared postfix,");
            return 0;
        }
    }

    /* if all operator and operands are valid, return 3 */
    return (strlen(num1) > 0) + (strlen(op) > 0) + (strlen(num2) > 0);
}

/* get the input and parse it into valid numbers and operators
 * then turn to specific calculations according to the operator 
 * and the symbols of the numbers
 */
int 
parse_and_calculate(const char *input, char *num1, char *operator, char *num2,int * num1_symbol,int * num2_symbol,int type){

    int count = parse_expression(input, num1, operator, num2,num1_symbol,num2_symbol);

    int result_size = strlen(num1) + strlen(num2);
    char * result = malloc(result_size * sizeof(char));
    char * tempt_result = malloc(result_size * sizeof(char));

    /* 0 0 ; 0 1 ; 1 0 ; 1 1 => 0 ; 1 ; 2 ; 3 total 4 distinct cases */
    int condition = (*num1_symbol)*(2) + *num2_symbol;
    if (count == 3) 
    {
        switch (*operator) 
        {
            case '+':
                switch (condition)
                {
                    case 0:/* 1 + 1 */
                        result = add(num1, num2);
                        break;
                    case 1:/* 1 + -1 */
                        result = subtract(num1,num2,0);
                        break;
                    case 2:/* -1 + 1 */
                        result = subtract(num2, num1,0);
                        break;
                    case 3:/* -1 + -1 */
                        /* add the minus symbol */
                        tempt_result = add(num1, num2);
                        strcpy(result + 1, tempt_result);
                        result[0] = '-';
                        break;
                    default:
                        printf("invalid symbol, ");
                        break;
                }
            break;
            case '-':
                switch (condition)
                {
                    case 0:/* 1 - 1*/
                        result = subtract(num1, num2,0);
                        break;
                    case 1:/* 1 - -1*/
                        result = add(num1,num2);
                        break;
                    case 2:/* -1 - 1*/
                        tempt_result = add(num1, num2);
                        strcpy(result + 1, tempt_result);
                        result[0] = '-';
                        break;
                    case 3:/* -1 - -1*/
                        result = subtract(num2, num1,0);
                        break;
                    default:
                        printf("invalid symbol, ");
                        break;
                }
                break;
            case 'x':
            case '*':
                switch (condition)
                {
                    case 0:/* 1 * 1*/
                    case 3:/* -1 * -1*/
                        result = multiply(num1, num2);
                        break;
                    case 1:/* 1 * -1*/
                    case 2:/* -1 * 1*/
                        tempt_result = multiply(num1, num2);
                        strcpy(result + 1, tempt_result);
                        result[0] = '-';
                        break;
                    default:
                        printf("invalid symbol, ");
                        break;
                }
                break;
            case '/':
                if (*num2 == '0' && strlen(num2) == 1) 
                {
                    printf("A number cannot be divided by zero, ");
                    return 0;
                }
                switch (condition)
                {
                    case 0:/* 1 / 1*/
                    case 3:/* -1 / -1*/
                        result = divide(num1, num2);
                        break;
                    case 1:/* 1 / -1*/
                    case 2:/* -1 / 1*/
                        tempt_result = divide(num1, num2);
                        strcpy(result + 1, tempt_result);
                        result[0] = '-';
                        break;
                    default:
                        printf("invalid symbol, ");
                        break;
                }
                break;
            default:
                printf("Unknown operator: %s,", operator);
                return 0;
            }

            /* prepare the char type symbol when output both operands and result */
            char symbol1 = '\0';
            char symbol2 = '\0';
            if(*num1_symbol)
                symbol1 = '-';
            if(*num2_symbol)
                symbol2 = '-';

            /* if the answer is -0 , then change it to 0 */
            if(result[0] == '-' && result[1] == '0' && strlen(result) == 2)
            {
                result[0] = '0';
                result[1] = '\0';
            }

            /* convert the answer to scientific notation */
            scientific_notation(result,(int)strlen(result),ANS_PRECISION,MAX_E);

            /* print the answer */
            if(type)
                printf("%c%s %s %c%s = %s\n",symbol1, num1, operator, symbol2, num2, result);
            else
            printf("%s\n",result);

            free(result);
            free(tempt_result);
            return 1;
    }
    else
    {
        printf("failed to parse the parameters, ");
        free(result);
        free(tempt_result);
        return 0;
    }
}

/* add two numbers by aligning all bits and add the bits one 
 * by one from low to high, then add the carry to the next bit
 */
char* 
add(const char * num1_str, const char * num2_str) 
{
    /* Record the length of interger parts and farction parts of both number */
    int num1_IntegerDigits = 0;
    int num1_FractionDigits = 0;
    int num2_IntegerDigits = 0;
    int num2_FractionDigits = 0;

    /* Find the position of '.' */
    char * num1_dot = strchr(num1_str, '.');
    char * num2_dot = strchr(num2_str, '.');
    if (num1_dot) 
    {
        num1_IntegerDigits = num1_dot - num1_str;
        num1_FractionDigits = strlen(num1_str) - num1_IntegerDigits - 1;
    }
    else
        num1_IntegerDigits = strlen(num1_str);

    if (num2_dot) 
    {
        num2_IntegerDigits = num2_dot - num2_str;
        num2_FractionDigits = strlen(num2_str) - num2_IntegerDigits - 1;
    }
    else
        num2_IntegerDigits = strlen(num2_str);


    /* store the number without its dot */
    int num_size = strlen(num1_str) + strlen(num2_str);
    char * num1_noDot = malloc(num_size * sizeof(char)); 
    char * num2_noDot = malloc(num_size * sizeof(char));
    memset(num1_noDot, 0, num_size * sizeof(char));
    memset(num2_noDot, 0, num_size * sizeof(char));
    num1_noDot[num_size-1] = '\0';
    num2_noDot[num_size-1] = '\0';

    int j = 0;
    for (int i = 0; i < strlen(num1_str); i++) 
    {
        if (num1_str[i] != '.')
            num1_noDot[j++] = num1_str[i];

    }

    j = 0;
    for (int i = 0; i < strlen(num2_str); i++) 
    {
        if (num2_str[i] != '.')
            num2_noDot[j++] = num2_str[i];

    }

    /* dig_len is the biggest possible length of the answer 
     * without the dot, add 1 for carry of the biggest bit
     */
    int dig_len = (int)(max(num1_IntegerDigits,num2_IntegerDigits)+max(num1_FractionDigits,num2_FractionDigits)+1);
    int ans_Digits[dig_len];
    memset(ans_Digits, 0, sizeof(ans_Digits));

    int num1_offset = (int)(max(num1_IntegerDigits,num2_IntegerDigits)) - num1_IntegerDigits;
    int num2_offset = (int)(max(num1_IntegerDigits,num2_IntegerDigits)) - num2_IntegerDigits;
    j = 0;
    for(int i = num1_offset ; i < num1_offset+num1_IntegerDigits+num1_FractionDigits ; i++)
        ans_Digits[i+1] += num1_noDot[j++] - '0';
    
    j = 0;
    for(int i = num2_offset ; i < num2_offset+num2_IntegerDigits+num2_FractionDigits ; i++)
        ans_Digits[i+1] += num2_noDot[j++] - '0';
    

    /* from the lowest bit, add bitwise and calculate the carry */
    for(int k = dig_len-1 ; k > 0 ; k--)
    {
        int num = ans_Digits[k];
        int carry = num / 10;
        int res = num - carry*10;

        ans_Digits[k] = res;
        ans_Digits[k-1] += carry;
    }

    free(num1_noDot);
    free(num2_noDot);

    int decimal_pos = (int)max(num1_FractionDigits,num2_FractionDigits);
    return trans_to_ans(ans_Digits,dig_len,decimal_pos);
}

/* Subtract two numbers by aligning all bits and subtract the bits one 
 * by one from low to high, if not enough, borrow ten from the higher bits.
 *
 * For parameter "int type": type=1 to return a answer without eliminating the
 * prefix char zeros, this is used in the divide fucntion;type=0 to return an
 * answer with prefix zeros eliminated.
 */
char* 
subtract(const char * num1_str,const char * num2_str,int type) 
{
    /* Record the length of interger parts and farction parts of both number */
    int num1_IntegerDigits = 0;
    int num1_FractionDigits = 0;
    int num2_IntegerDigits = 0;
    int num2_FractionDigits = 0;

    /*Find the position of '.' */
    char * num1_dot = strchr(num1_str, '.');
    char * num2_dot = strchr(num2_str, '.');

    if (num1_dot) 
    {
        num1_IntegerDigits = num1_dot - num1_str;
        num1_FractionDigits = strlen(num1_str) - num1_IntegerDigits - 1;
    } 
    else
        num1_IntegerDigits = strlen(num1_str);

    if (num2_dot)
    {
        num2_IntegerDigits = num2_dot - num2_str;
        num2_FractionDigits = strlen(num2_str) - num2_IntegerDigits - 1;
    } 
    else
        num2_IntegerDigits = strlen(num2_str);


    /* in order to shift the bit left or right conveniently,
     * initialize all bits to char zero 
     */
    int num_size = strlen(num1_str) + strlen(num2_str);
    char * num1_noDot = malloc(num_size * sizeof(char)); 
    char * num2_noDot = malloc(num_size * sizeof(char));
    memset(num1_noDot, '0', num_size * sizeof(char));
    memset(num2_noDot, '0', num_size * sizeof(char));
    num1_noDot[num_size-1] = '\0';
    num2_noDot[num_size-1] = '\0';

    int j = 0;
    for (int i = 0; i < strlen(num1_str); i++) 
    {
        if (num1_str[i] != '.') 
            num1_noDot[j++] = num1_str[i];
        
    }

    j = 0;
    for (int i = 0; i < strlen(num2_str); i++) 
    {
        if (num2_str[i] != '.')
            num2_noDot[j++] = num2_str[i];

    }

    /* To make the calculation convenient, we want set the bigger number as 
     * the minuend and the smaller one as subtrahend, so the following
     * code calculates the offsets for alignment and make the bigger one
     * as num1, smaller one as num2
     */
    int num2_offset = (int)(num1_IntegerDigits - num2_IntegerDigits);
    int num1_offset = (int)(num2_IntegerDigits - num1_IntegerDigits);
    int num1_bge_num2 = 1;
    if(num1_IntegerDigits >= num2_IntegerDigits)
    {
        for(int i = strlen(num2_noDot)-num2_offset-1 ; i >= 0 ; i--)
            num2_noDot[i+num2_offset] = num2_noDot[i];
        
        for(int i = 0 ; i < num2_offset ; i++)
            num2_noDot[i] = '0';
        
        if(strcmp(num1_noDot,num2_noDot)<0)
            num1_bge_num2 = 0;

    }
    else
    {
        for(int i = strlen(num1_noDot)-num1_offset-1 ; i >= 0 ; i--)
            num1_noDot[i+num1_offset] = num1_noDot[i];
        
        for(int i = 0 ; i < num1_offset ; i++)
            num1_noDot[i] = '0';
        
        if(strcmp(num1_noDot,num2_noDot)<0)
            num1_bge_num2 = 0;
        
    }

    if(!num1_bge_num2)
    {
        int t1 = num1_IntegerDigits;
        num1_IntegerDigits = num2_IntegerDigits;
        num2_IntegerDigits = t1;

        int f1 = num1_FractionDigits;
        num1_FractionDigits = num2_FractionDigits;
        num2_FractionDigits = f1;

        char * tempt_noDot = malloc(num_size * sizeof(char));
        memset(tempt_noDot, '0', num_size * sizeof(char));
        tempt_noDot[num_size-1] = '\0';

        for (int i = 0; i < strlen(num1_noDot); i++)
            tempt_noDot[i] = num1_noDot[i];
        
        for (int i = 0; i < strlen(num2_noDot); i++)
            num1_noDot[i] = num2_noDot[i];
        
        for (int i = 0; i < strlen(tempt_noDot); i++)
            num2_noDot[i] = tempt_noDot[i];
        
        free(tempt_noDot);
    }

    /* dig_len is similar as add function, but we don't need the carry bit */
    int dig_len = (int)(max(num1_IntegerDigits,num2_IntegerDigits)+max(num1_FractionDigits,num2_FractionDigits));
    int ans_Digits[dig_len];
    memset(ans_Digits, 0, sizeof(ans_Digits));

    for(int i = 0 ; i < num1_IntegerDigits+num1_FractionDigits ; i++)
        ans_Digits[i] += num1_noDot[i] - '0';
    

    /* do the subtraction */
    for(int k = dig_len-1 ; k >= 0 ; k--)
    {
        int num = ans_Digits[k];
        int subtractor = 0;

        if(num2_noDot[k] == 0)
            subtractor = 0;
        else
            subtractor = num2_noDot[k] - '0';
        
        if(num < subtractor)
        {
            num+=10;
            ans_Digits[k-1]--;
        }
        ans_Digits[k] = num - subtractor;
    }


    /* produce the answer according to type */
    int decimal_pos = (int)max(num1_FractionDigits,num2_FractionDigits);
    char *answer;
    if(type == 0)
        answer = trans_to_ans(ans_Digits,dig_len,decimal_pos);
    else
    {
        answer = malloc((dig_len + 2) * sizeof(char)); 
        int idx = 0;
        for (int i = 0; i < dig_len; i++) 
        {
            if (i == dig_len - decimal_pos) 
                answer[idx++] = '.';
            
            answer[idx++] = ans_Digits[i] + '0';
        }
        if(!idx)
            answer[idx++] = '0';
        answer[idx] = '\0';    
    }

    /* if we swap num1 and num2 beforehead, add back the minus symbol */
    char * tempt_answer = malloc((dig_len + 3) * sizeof(char)); 
    if(!num1_bge_num2)
    {
        strcpy(tempt_answer + 1, answer);
        tempt_answer[0] = '-';
        free(answer);
        return tempt_answer;
    }

    free(tempt_answer);
    free(num1_noDot);
    free(num2_noDot);

    return answer;
}

/* multiply two numbers by multiplying one bit in a number with all 
 * bits in another number one by one, and repeatedly do the process
 * for all bits in the first number.
 */
char* 
multiply(const char * num1_str,const char * num2_str)
 {

    /* Record the length of interger parts and farction parts of both number */
    int num1_IntegerDigits = 0;
    int num1_FractionDigits = 0;
    int num2_IntegerDigits = 0;
    int num2_FractionDigits = 0;

    /* Find the position of '.' */
    char * num1_dot = strchr(num1_str, '.');
    char * num2_dot = strchr(num2_str, '.');

    if (num1_dot) 
    {
        num1_IntegerDigits = num1_dot - num1_str;
        num1_FractionDigits = strlen(num1_str) - num1_IntegerDigits - 1;
    } 
    else
        num1_IntegerDigits = strlen(num1_str);
    

    if (num2_dot) 
    {
        num2_IntegerDigits = num2_dot - num2_str;
        num2_FractionDigits = strlen(num2_str) - num2_IntegerDigits - 1;
    } 
    else
        num2_IntegerDigits = strlen(num2_str);


    int num_size = strlen(num1_str) + strlen(num2_str);
    char * num1_noDot = malloc(num_size * sizeof(char)); 
    char * num2_noDot = malloc(num_size * sizeof(char));

    int j = 0;
    for (int i = 0; i < strlen(num1_str); i++)
    {
        if (num1_str[i] != '.')
            num1_noDot[j++] = num1_str[i];

    }

    j = 0;
    for (int i = 0; i < strlen(num2_str); i++)
    {
        if (num2_str[i] != '.')
            num2_noDot[j++] = num2_str[i];

    }

    /* dig_len is the possible biggest length of the answer without dot, which,
     * different from add and subtract, needs to add all length of Integer and Fraction part
     * of both numbers
     */
    int dig_len = num1_IntegerDigits+num2_IntegerDigits+num1_FractionDigits+num2_FractionDigits;
    int ans_Digits[dig_len];
    memset(ans_Digits, 0, sizeof(ans_Digits));

    /* use double for loop to do the calculation */
    for(int i = 0 ; i < num1_IntegerDigits+num1_FractionDigits ; i++)
    {
        int num1_cur_digit = num1_noDot[num1_IntegerDigits+num1_FractionDigits - i - 1] - '0';
        for(int j = 0 ; j < num2_IntegerDigits+num2_FractionDigits ; j++)
        {
            int num2_cur_digit = num2_noDot[num2_IntegerDigits+num2_FractionDigits - j - 1] - '0';
            int ind = num1_IntegerDigits + num2_IntegerDigits + num1_FractionDigits + num2_FractionDigits - (i+j+1);
            ans_Digits[ind] += num1_cur_digit * num2_cur_digit;
        }
    }

    /* calculate the number on each bit(might be bigger than 10) 
     * and add the carry to next the next bit
     */
    for(int k = num1_IntegerDigits+num2_IntegerDigits+num1_FractionDigits+num2_FractionDigits - 1 ; k > 0 ; k--)
    {
        int num = ans_Digits[k];
        int carry = num / 10;
        int res = num - carry*10;

        ans_Digits[k] = res;
        ans_Digits[k-1] += carry;
    }

    free(num1_noDot);
    free(num2_noDot);
    int decimal_pos = (int)(num1_FractionDigits+num2_FractionDigits);
    return trans_to_ans(ans_Digits,dig_len,decimal_pos);
}

/* Divide two numbers by repeatedly subtract the divisor from the 
 * dividend, and record how many times we subtracted it.
 *
 * The function is optimized by in each subtraction, shift
 * the divisor(multiply or divide 10) to be exactly smaller
 * than the dividend, and the record time for this 
 * subtraction will be timed with ten's power of offset,
 * This greatly improves the efficiency when the quotient is very large
 */
char* 
divide(const char * num1_str,const char * num2_str) 
{

    /* Record the length of interger parts and farction parts of both number */
    int num1_IntegerDigits = 0;
    int num1_FractionDigits = 0;
    int num2_IntegerDigits = 0;
    int num2_FractionDigits = 0;

    /* Find the position of '.' */
    char * num1_dot = strchr(num1_str, '.');
    char * num2_dot = strchr(num2_str, '.');
    if (num1_dot) 
    {
        num1_IntegerDigits = num1_dot - num1_str;
        num1_FractionDigits = strlen(num1_str) - num1_IntegerDigits - 1;
    }
    else
        num1_IntegerDigits = strlen(num1_str);

    if (num2_dot) 
    {
        num2_IntegerDigits = num2_dot - num2_str;
        num2_FractionDigits = strlen(num2_str) - num2_IntegerDigits - 1;
    } 
    else
        num2_IntegerDigits = strlen(num2_str);


    /* initialize the numbers with char zeros is important
     * concerning the shift operation
     */
    int num_size = strlen(num1_str) + strlen(num2_str) + BASE_PRECISION;
    char * num1_noDot = malloc(num_size * sizeof(char)); 
    char * num2_noDot = malloc(num_size * sizeof(char));
    memset(num1_noDot, '0', num_size * sizeof(char));
    memset(num2_noDot, '0', num_size * sizeof(char));
    num1_noDot[num_size-1] = '\0';
    num2_noDot[num_size-1] = '\0';
    
    int j = 1;
    for (int i = 0; i < strlen(num1_str); i++)
    {
        if (num1_str[i] != '.') 
            num1_noDot[j++] = num1_str[i];
        
    }

    j = 1;
    for (int i = 0; i < strlen(num2_str); i++) 
    {
        if (num2_str[i] != '.')
            num2_noDot[j++] = num2_str[i];

    }

    int dig_len = (int)(max(num1_IntegerDigits,num2_IntegerDigits)+max(num1_FractionDigits,num2_FractionDigits));
    int ans_Digits[dig_len];
    memset(ans_Digits, 0, sizeof(ans_Digits));

    /* shift the smaller number to align with the larger number,
     * the null caused by shifting is filled with char zeros again
     */
    int num2_offset = (int)(num1_IntegerDigits - num2_IntegerDigits);
    int num1_offset = (int)(num2_IntegerDigits - num1_IntegerDigits);
    if(num1_IntegerDigits > num2_IntegerDigits)
    {
        for(int i = strlen(num2_noDot)-num2_offset-1 ; i >= 0 ; i--)
            num2_noDot[i+num2_offset] = num2_noDot[i];
        
        for(int i = 0 ; i < num2_offset ; i++)
            num2_noDot[i] = '0';
        
    }
    else
    {
        for(int i = strlen(num1_noDot)-num1_offset-1 ; i >= 0 ; i--)
            num1_noDot[i+num1_offset] = num1_noDot[i];
        
        for(int i = 0 ; i < num1_offset ; i++)
            num1_noDot[i] = '0';
        
    }

    for(int i = 0 ; i < num1_IntegerDigits+num1_FractionDigits ; i++)
        ans_Digits[i] += num1_noDot[i] - '0';
    

    char * quotient = malloc((BASE_PRECISION) * sizeof(char)); 
    quotient[0] = '0';

    //set the precision of the result
    char * epsilon = malloc(BASE_PRECISION * sizeof(char)); 
    memset(epsilon, '0', BASE_PRECISION * sizeof(char));
    epsilon[BASE_PRECISION-2] = '1';
    epsilon[BASE_PRECISION-1] = '\0';

    /* Loop while the dividend is still larger than a very small number epsilon.
     * power_offset records how many bits the divisor has shifted from its origin
     */
    int power_offset = 0;
    while(strcmp(num1_noDot,epsilon)>0)
    {
        if(strcmp(num1_noDot,num2_noDot)>0)
        {
            /* shift the divisor left until just bigger than dividend */
            while(strcmp(num1_noDot,num2_noDot)>0)
            {
                for(int i = 0 ; i < strlen(num2_noDot)-2 ; i++)
                    num2_noDot[i] = num2_noDot[i+1];
                power_offset++;
            }

            /* shift the divisor right for one bit to make divisor just smaller than dividend*/
            for(int i = strlen(num2_noDot)-2 ; i >= 0 ; i--)
            {
                num2_noDot[i+1] = num2_noDot[i];
                num2_noDot[i] = '0';
            }
            power_offset--;

        }
        else if(strcmp(num1_noDot,num2_noDot)<0)
        {
            /* shift the divisor right until just smaller than dividend */
            while(strcmp(num1_noDot,num2_noDot)<0)
            {
                for(int i = strlen(num2_noDot)-2 ; i >= 0 ; i--)
                {
                    num2_noDot[i+1] = num2_noDot[i];
                    num2_noDot[i] = '0';
                }
                power_offset--;
            }
        }

        /* add more on quotient with ten's power of power_offset */
        quotient = add(quotient,tens_power_of(power_offset));
        /* do the subtraction */
        strcpy(num1_noDot,subtract(num1_noDot,num2_noDot,1));
    }
    free(num1_noDot);
    free(num2_noDot);
    free(epsilon);
    return quotient;
}

/* calculate the square root of parameter a
 * by using Newton's iterative method,
 * the add, subtract, and divide functions
 * are the ones above, so theoretically this
 * function can also deal with big numbers
 */
char* 
square_root(const char * a) {

    /* the epsilon number from float.h */
    char * epsilon = "0.00000000000000022204460492503131";

    /* number_size dicides the precison during calculations */
    char * num = malloc(OPERAND_MAX_SIZE * sizeof(char));
    char * x = malloc(OPERAND_MAX_SIZE * sizeof(char));
    char * prev_x = malloc(OPERAND_MAX_SIZE * sizeof(char));
    strcpy(num,a);
    strcpy(x,num);

    char * delta = malloc(OPERAND_MAX_SIZE * sizeof(char));

    do 
    {
        /* prev_x = x */
        strcpy(prev_x,x);

        /* Newton's iterative function : x = (x + a / x) / 2 */
        char two[] = "2";
        char * ans1 = divide(num,x);
        char * ans2 = add(ans1,x);
        char * ans3 = divide(ans2,two);
        strcpy(x,ans3);

        /* calculate the difference : delta = x - prev_x */
        strcpy(delta,subtract(x,prev_x,0));

        /* if(delta < 0) delta = -delta */
        if(delta[0] == '-')
        {
            for(int i = 1 ; i < strlen(delta) ; i++)
                delta[i-1] = delta[i];
            
            delta[strlen(delta)-1] = '\0';
        }
    
    } 
    while (strcmp(delta,epsilon) > 0);// stop iteration when difference is smaller than some very small number
   
    /* some supplement if the answer be like x.9999... 
     * add the last bit to 10(for numbers with long fraction bis) 
     * and calculate the carry upwards.
     * 
     * At last, eliminate the long char zeros after dot if the true
     * answer is an integer.
     */
    if(strlen(x) > 30)
    {  
        char * x_dot = strchr(x, '.');
        if(x_dot)
        {
            int x_Frac_len = strlen(x) - (x_dot - x);
            int last_digit = x[strlen(x)-1] - '0';
            if(last_digit > 0)
            {
                char * new_x = malloc(OPERAND_MAX_SIZE * sizeof(char));
                strcpy(new_x,x);
                char minimum[] = {(10-last_digit)+'0','\0'};
                new_x = add(x,multiply(minimum,tens_power_of(1-x_Frac_len)));

                int dot_flag = 0;
                for(int i = strlen(new_x)-1 ; i>=0 ; i--)
                {
                    if((new_x[i]=='0' || new_x[i] == '.') && !dot_flag)
                    {
                        if(new_x[i]=='.')
                            dot_flag = 1;
                        new_x[i] = '\0';
                    }
                    else
                        break;
                    
                }
                free(prev_x);
                free(num);
                free(x);
                free(delta);
                return new_x;
            }
        }
    }

    free(prev_x);
    free(num);
    free(delta);
    return x;
}

/* return bigger number between a and b*/
double 
max(double a , double b){
    return a > b ? a : b;
}

/* return smaller number between a and b*/
double 
min(double a , double b){
    return a > b ? b : a;
}

/* translate a number array that store all bits of the answer 
 * into a string answer with dot and eliminate prefix zeros
 */
char* 
trans_to_ans(int ans_Digits[],int dig_len,int decimal_pos)
{

    /* answer from int arrays to string with dot */
    char * answer = malloc((dig_len + 2) * sizeof(char)); 
    int idx = 0;
    for (int i = 0; i < dig_len; i++) 
    {
        if (i == dig_len - decimal_pos)
            answer[idx++] = '.';
        
        answer[idx++] = ans_Digits[i] + '0';
    }

    /* true_answer eliminates the prefix zeros */
    char * true_answer = malloc((dig_len + 2) * sizeof(char)); 
    int tid = 0;
    int flag = 0;
    for(int i = 0 ; i < idx ; i++)
    {
        if(answer[i] != '0')
            flag = 1;
        if(!flag && answer[i] == '0')
            continue;
    
        true_answer[tid++] = answer[i];
    }

    /* if all zero, supplement a zero */
    if(!tid)
        true_answer[tid++] = '0';
    true_answer[tid] = '\0';
    
    /* if form like .xxx , supplement a zero */
    if(true_answer[0] == '.')
    {
        char * new_answer = malloc((dig_len + 3) * sizeof(char)); 
        strcpy(new_answer+1,true_answer);
        new_answer[0] = '0';
        free(answer);
        free(true_answer);
        return new_answer;
    }

    free(answer);
    return true_answer;
}

/* return a string of 10's power */
char* 
tens_power_of(int power) 
{
    int size_p = power < 0 ? -power : power;
    char * answer;
    /* produce the number by simply manipulate the string */
    if(power < 0)
    {
        answer = calloc((size_p+3), sizeof(char));
        memset(answer, '0', size_p+2);
        answer[1] = '.';
        answer[size_p+1] = '1';
        answer[size_p+2] = '\0';
    }
    else
    {
        answer = calloc((size_p+2), sizeof(char));
        memset(answer, '0', size_p+1);
        answer[0] = '1';
        answer[size_p+1] = '\0';
    }
    
    return answer;
}

/* translate the answer that has its first valid number too far 
 * away (parameter max_e decides its scale) from its decimal point 
 * into scientific notation(can dicide it precision with parameter)
 */
int 
scientific_notation(char * answer, int ans_len, unsigned int precision, int max_e)
{

    /* preprocess the answer string by eliminating the symbol first */
    char symbol = ' ';
    if (answer[0] == '-' || answer[0] == '+') 
    {
        symbol = answer[0];
        for (int i = 0; i < ans_len - 1; i++)
            answer[i] = answer[i + 1];
        
        answer[ans_len - 1] = '\0';
        ans_len--;
    }

    /* check the offset between the answer's first valid number and its decimal point*/
    char * ans_dot = strchr(answer, '.');
    char * numbers = "123456789";
    int offset = 0;
    if (ans_dot) 
    {
        offset = ans_dot - answer - 1;
        if(offset==0 && answer[0]=='0')
        {
            char * ans_dot = strpbrk(answer, numbers);
            if(ans_dot)
                offset = -(ans_dot - answer - 1);
            else
                offset = 0;
            
        }
    } 
    else
        offset = ans_len-1;
    

    /* if the offset absolutely exceeds max_e, translate it */
    max_e = max_e < 0 ? -max_e : max_e;
    if(offset > max_e || offset < -max_e)
    {

        /* multiply the answer by the ten's power of offset to get the answer
         * with decimal point at its second position
         * than copy the prefix numbers according to precision
         */
        char * new_ans = malloc((ans_len + 1) * sizeof(char));
        char * multiplied_ans = multiply(answer, tens_power_of(-offset));
        strncpy(new_ans, multiplied_ans, precision+2);
        new_ans[precision+2] = '\0';

         /* add the symbol and the prefix answer */
        char final_answer[100] = {0};
        if (symbol != ' ') 
            sprintf(final_answer, "%c%s", symbol, new_ans);
        else
            sprintf(final_answer, "%s", new_ans);

        /* add the e and offset notation at the back */
        char off[10];
        sprintf(off, "e%d", offset);
        strcat(final_answer, off);

        /* copy the true answer back to answer*/
        strncpy(answer, final_answer, ans_len);
        answer[ans_len] = '\0';

        /*free memory*/
        free(new_ans);
        free(multiplied_ans);
    /* if not, just put back the symbol and pretend nothing happend */
    }
    else 
    {
        if (symbol != ' ') 
        {
            char * final_answer = malloc((ans_len + 2) * sizeof(char));
            sprintf(final_answer, "%c%s", symbol, answer);
            strncpy(answer, final_answer, ans_len + 1);
            free(final_answer);
        }
    }

    return 0;
}

/* sin function: based on Talor expansion */
char * 
sine(const char * a,int * is_minus)
{

    while(subtract(a,"360",0)[0] != '-')
    {
        a=subtract(a,"360",0);
    }

    if(subtract(a,"90",0)[0] != '-' && subtract(a,"180",0)[0] == '-' )
        a=subtract("180",a,0);
    else if(subtract(a,"180",0)[0] != '-' && subtract(a,"270",0)[0] == '-')
    {
        if(strcmp(a,"180")!=0)
            *is_minus = 1;
        a=subtract(a,"180",0);
    }
    else if(subtract(a,"270",0)[0] != '-' && subtract(a,"360",0)[0] == '-')
    {
        if(strcmp(a,"360")!=0)
            *is_minus = 1;
        a=subtract("360",a,0);
    }


    /* convert the input into radian form */
    char * radians = divide(multiply(a, PI), "180"); 

    /* initialize */
    char *x = malloc(OPERAND_MAX_SIZE * sizeof(char));
    char *result = malloc(OPERAND_MAX_SIZE * sizeof(char));
    char *factorial = malloc(OPERAND_MAX_SIZE * sizeof(char));
    char *fact_n_str = malloc(OPERAND_MAX_SIZE * sizeof(char));

    strcpy(x, radians);
    strcpy(result, x);
    strcpy(factorial, "1");

    int fact_n = 2;
    int sign = -1;

    for (int i = 0; i < TAYLOR_EXPANSION_MAX_ORDER; i++) 
    {
        /* in each iterations, x = x * radian * radian */
        x = multiply(multiply(x, radians), radians);

        /* in each iterations, fact_value *= (fact_n+1) * (fact_n+2) */
        snprintf(fact_n_str, OPERAND_MAX_SIZE, "%d", fact_n++);
        factorial = multiply(factorial, fact_n_str);
        snprintf(fact_n_str, OPERAND_MAX_SIZE, "%d", fact_n++);
        factorial = multiply(factorial, fact_n_str);
        
        result = (sign == 1) ? add(result, divide(x, factorial)) : subtract(result, divide(x, factorial), 0);
        sign *= -1;
    }

    free(x);
    free(factorial);
    free(fact_n_str);
    return result;
}

