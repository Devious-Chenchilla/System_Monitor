 #include <stdio.h>
    #include <conio.h>
    #include <string.h>
     //library for sleep() function
    #include <unistd.h>

    void main()
    {
     int i,j,n,k;

    //text to be srolled
     char t[30]="akhil is a bad boy";
     n=strlen(t);
     for(i=0;i<n;i++)
     {
        printf("\n");
//loop for printing spaces
        for(j=20-i;j>0;j--)
        {
            printf(" ");
        }

/*printing text by adding a character every time*/
        for(k=0;k<=i;k++)
        {
            printf("%c",t[k]);
        }
// clearing screen after every iteration
        sleep(1);
        if(i!=n-1)
        clrscr();

     }
