//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// описание внешних подпрограмм и классов
#include "Main_system.h"  //заголовок программы - подключение функций и классов
//---------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
//——————————————————————————————————————————————————————————————————————————————
//Вспомогательные переменные
int i,j,k;                 //счётчики
int ier;                   //номер ошибки

int count1;                //счётчик
int count2;                //счётчик
//--------------------------
//переменные
double t;                  //время

int pr_slojov;             //счётчик временных слоёв

double I_min;              //самый отрицательный ток в диодах (это ребро отключится первым)
int I_min_num;             //номер самого "отрицательного" диода

bool pr_diod_wrong;        //признак наличия отрицательных диодов
bool pr_pechat;            //признак печати матрицы СЛАУ

//——————————————————————————————————————————————————————————————————————————————
//Открытие файлов исходных данных и файлов для печати результатов.
//--------------------------
//Переменные открытия файлов
int NF = 9;                                //Количество открываемых файлов

const char **fil = new const char *[NF];               //Массив названий файлов
FILE **f = new FILE *[NF];                 //Массив файлов

fil[0]="E:\\Data_All\\RT\\RT_Voda.txt";			//исходные данные для рабочего тела

fil[5]="..\\..\\Data\\RebraData.txt";			//файл исходных данных
fil[6]="..\\..\\Data\\ControlData.txt";			//файл исходных данных

fil[1]="..\\..\\Rez\\FinalData.txt";            //резерв
fil[2]="..\\..\\Rez\\CurrentData.txt";			//файл результатов - напряжения
fil[3]="..\\..\\Rez\\UzelData.txt";				//резерв
fil[4]="..\\..\\Rez\\Reserve.txt";				//резерв
fil[7]="..\\..\\Rez\\Reserve2.txt";				//файл результатов - токи
fil[8]="..\\..\\Rez\\Misc.txt";					//Разное
//--------------------------
for(i=0; i < NF; i++)
		{
		if((f[i]=fopen(fil[i],(i==0 || i==5 || i==6)? "rt":"wt"))==0)
				{
				std::cout<<"Couldn't open file:"<<i<<" . Press any key to quit";
				std::cin.get();
				return(-1);
				}
		}

//——————————————————————————————————————————————————————————————————————————————
//Читение заголовка
 double *HeaderData = new double[20];          //создания массива для записи прочитанных данных

ier = NewReadHeader(f[6], HeaderData);         //вызов функции чтения данных

//запись данных в переменнык
int 		Uzel_kol	= (int)	HeaderData[0]; //количество узлов в графе
int 		Rebro_kol	= (int)	HeaderData[1]; //количество рёбер в графе
double		dt_mks		= 		HeaderData[5]; //шаг по времени
int			N_slojov	= (int)	HeaderData[6]; //максимальное кол-во шагов по времени
double		faza0		= 		HeaderData[7]; //начальная фаза генератора

delete HeaderData;                             //удаление массива

//——————————————————————————————————————————————————————————————————————————————
//Инициализация массивов экземпляров классов узлов и рёбер
//--------------------------
//набор массивов экземпляров классов узлов для разных положений ключей
//(в данный момент используется только один набор и он постоянно пересчитывается)
typedef Class_Uzel_Elektrik *Sizeof_Class_Uzel_Elektrik;
Sizeof_Class_Uzel_Elektrik **Class_Uzel = new Sizeof_Class_Uzel_Elektrik *[2];
for (i = 0; i < 2; i++)                  //Цикл по всем конфигурациям узлов(не используются)
{
	Class_Uzel[i] = new Sizeof_Class_Uzel_Elektrik[Uzel_kol];
	for (j = 0; j < Uzel_kol; j++)       //Цикл по всем узлам
	{
		Class_Uzel[i][j] = new Class_Uzel_Elektrik();
	}
}
//--------------------------
//массив экземпляров классов рёбер
typedef Class_Rebro_Elektrik *Sizeof_Class_Rebro_Elektrik;
Sizeof_Class_Rebro_Elektrik *Class_Rebro = new Sizeof_Class_Rebro_Elektrik[Rebro_kol];
//--------------------------
//набор массивов замкнутых контуров - используются только два: 	1) всё включено
//																2) не всё включено
Kontur *Kontur_base = new Kontur();
Kontur *Kontur_mod = new Kontur();
//——————————————————————————————————————————————————————————————————————————————
//создание матрицы СЛАУ, вектора правой части и вектора переменных
double **matrix_a = new double *[Rebro_kol];
for (i = 0; i < Rebro_kol; i++)          //Цикл по всем рёбрам
{
	matrix_a[i] = new double [Rebro_kol];
}

double *vector_b = new double [Rebro_kol];
double *vector_F = new double [Rebro_kol];

//--------------------------
//массив комбинаций ключей
int *kluch_kombo = new int[Rebro_kol];
//обработанный массив комбинаций ключей (ичключены рёбра, которые остались оборванными
//(т.е. не входящими не в один замкнутый контур), после первой комбинации ключей
int *kluch_kombo_mod = new int[Rebro_kol];

//——————————————————————————————————————————————————————————————————————————————
//Активация классов рёбер (чтение их исходных данных в конструкторе класса рёбер
for (i = 0; i < Rebro_kol; i++)         //Цикл по всем рёбрам
{
	Class_Rebro[i] = new Class_Rebro_Elektrik(f[5], i);
}

//——————————————————————————————————————————————————————————————————————————————
//печать заголовка (названий) в файл
//ier = Print_current_header2(f[2],f[7], Uzel_kol, Rebro_kol, t, Class_Uzel[0], Class_Rebro);

//——————————————————————————————————————————————————————————————————————————————
//Основной цикл по времени
//——————————————————————————————————————————————————————————————————————————————
for (i = 0; i < Rebro_kol; i++) 			//Цикл по всем рёбрам
{
	kluch_kombo[i] = 1;          //расчёт коэффициентов A и B
}
//--------------------------
//поиск замкнутых контуров - все диоды замкнуты
ier = Find_kontur_array(Rebro_kol, Uzel_kol, kluch_kombo, kluch_kombo_mod, Class_Uzel[0],
		Class_Rebro, Kontur_base, f[3],pr_pechat);
//--------------------------

fprintf(f[8],"R \t U \t I \n");
Class_Rebro[1]->R_rebra_ini = 0.0575;
//Class_Rebro[1]->L_rebra_nonst = 0.00000575;
Class_Rebro[1]->C_rebra_nonst = 0.0023;
for (int i_vah = 0; i_vah < 60; i_vah++)
{

double U_max_vah=0;
double I_max_vah=0;

Class_Rebro[1]->R_rebra_ini = Class_Rebro[1]->R_rebra_ini*1.2;
//Class_Rebro[1]->L_rebra_nonst = Class_Rebro[1]->L_rebra_nonst*1.2;
Class_Rebro[1]->C_rebra_nonst = Class_Rebro[1]->C_rebra_nonst/1.2;
t = 0.0;           				//начало временного отчёта
pr_slojov = 0;     				//начало отсчёта временных слоёв
count1 = 0;        				//начало отсчёта для служебно счётчика (используется для моделирования процессов)
count2 = 1000;        				//начало отсчёта для служебно счётчика (используется для моделирования процессов)


while(pr_slojov<N_slojov)       //цикл по времени (ограничение по кол-ву шагов)
{

//--------------------------
//Эмуляция работы генератора - три симметричные фазы
//Class_Rebro[0]->EDS_nonst = (double)-220.0;

//Class_Rebro[0]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0))*220*1.41)/1.0;

Class_Rebro[0]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0))*2000*1.41);
//Class_Rebro[1]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0+1.0/3.0))*2000*1.41);
//Class_Rebro[2]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0+2.0/3.0))*2000*1.41);

//Class_Rebro[0]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0))*2000*1.41)/2.0;
//Class_Rebro[1]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0+1.0/3.0))*2000*1.41)/2.0;
//Class_Rebro[2]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0+2.0/3.0))*2000*1.41)/2.0;
//Class_Rebro[3]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0))*2000*1.41)/2.0;
//Class_Rebro[4]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0+1.0/3.0))*2000*1.41)/2.0;
//Class_Rebro[5]->EDS_nonst = (double)(sin(2*PI*(1000.0*t+faza0+2.0/3.0))*2000*1.41)/2.0;

//——————————————————————————————————————————————————————————————————————————————
////Программа запуска
//if (fabs(t-0.01) < 1.0e-12)
//{
//dt_mks = dt_mks/1000.0;
//}
////--------------------------
////размыкание ключа на нагрузке - увеличение сопротивления на ребра за 50 шагов
//if (t>0.01 && count1 < 1000)     //начать замыкание, при t>0.01с с начала эксперимента
//{
//
//	//увеличение сопротивления нагрузки в 1,1 раза 50 раз - один раз в шаг
//	//(при dt = 1 мкс размыкание происходит за 50 мкс)
//	Class_Rebro[18]->R_rebra_nonst = 575.0+100.0*(count1+1);
//	count1++;                 //инкримент счётчика размыкания - считает 50 раз
//
//}

//if (fabs(t-0.010002) < 1.0e-12)
//{
//dt_mks = dt_mks*1000.0;
//}
////

//if (fabs(t-0.0043) < 1.0e-12)
//{
//dt_mks = dt_mks/1000.0;
//Class_Rebro[10]->R_rebra_ini = 0.001;
//}
//
//if (fabs(t-0.004302) < 1.0e-12)
//{
//dt_mks = dt_mks*1000.0;
//}


//——————————————————————————————————————————————————————————————————————————————
//Поиск замкнутых контуров и составление матрицы СЛАУ Кирхгофа и её решение
//——————————————————————————————————————————————————————————————————————————————
//--------------------------
for (i = 0; i < Rebro_kol; i++) 			//Цикл по всем рёбрам
{
	Class_Rebro[i]->R_rebra_nonst = Class_Rebro[i]->R_rebra_ini;
}
//--------------------------

//расчёт коефициентов A и B. Считается, что каждое ребро при dt->0 обладает
//линейной зависимостью напряжения и тока: U = A*I + B
for (i = 0; i < Rebro_kol; i++) 			//Цикл по всем рёбрам
{
	Class_Rebro[i]->dt_nonst = dt_mks;      //установка временного шага этого временного слоя
	Class_Rebro[i]->Rash_new_AB();          //расчёт коэффициентов A и B
}

//--------------------------
//Создание и решение матрицы СЛАУ Кирхгофа
ier = Make_lin_system(Rebro_kol, Uzel_kol, vector_b, matrix_a, vector_F,
				Class_Uzel[0], Class_Rebro,	Kontur_base, kluch_kombo, f[4], pr_pechat);

//--------------------------
//проверка правильности состояния диодов
pr_diod_wrong=true;      //изначально диоды считаются неправильными для входа в цикл

//цикл пока все не станут правильными или не будет достигнут максимум итераций (10)
for (k = 0; (k < 100) && pr_diod_wrong; k++)
{
//Отключение диодов происходит по очереди, начиная с самого "худшего",
//т.е. с максимально отрицательным током
I_min = (double) 0.0; 		//инициализация переменной максимально отрицательного тока
I_min_num = -1;             //номер ребра с максимально отрицательным током
pr_diod_wrong = false;      //диоды считаются правильными до полной проверки
for (i = 0; i < Rebro_kol; i++)          //перебор по всем рёбрам
{
	//если ребро диод и ток по нему отрицательный, то заходим
	if(vector_F[i]<-1.0e-2 && Class_Rebro[i]->pr_D==1)
	{
	//ток по нему меньше, чем прежде записанный максимально отрицательный, то заходим
	if(vector_F[i]<I_min)
	{
		I_min = vector_F[i];   //запись нового максимально отрицательного тока
		I_min_num = i;         //запись номера ребра нового максимально отрицательного тока
	}
	pr_diod_wrong = true;      //Были диоды с отрицательным током - нужно отключить самый отрицательный
	}
}

//--------------------------
//Если "неправильных" диодов небыло - выйти и перейти к записи результатов
if(!pr_diod_wrong) break;

//--------------------------
Class_Rebro[I_min_num]->R_rebra_nonst = 1000000.0;   //отключение самого "худшего" диода
//——————————————————————————————————————————————————————————————————————————————
//расчёт коефициентов A и B. Считается, что каждое ребро при dt->0 обладает
//линейной зависимостью напряжения и тока: U = A*I + B
for (i = 0; i < Rebro_kol; i++) 			//Цикл по всем рёбрам
{
	Class_Rebro[i]->Rash_new_AB();          //расчёт коэффициентов A и B
}

//--------------------------
//создание и решение СЛАУ Кирхгофа
ier = Make_lin_system(Rebro_kol, Uzel_kol, vector_b, matrix_a, vector_F,
				Class_Uzel[0], Class_Rebro,	Kontur_base, kluch_kombo, f[4], pr_pechat);

//--------------------------
if (k==99) return 1;   //выход с ошибкой, если не нашёл нужной комбинации за 10 попыток
} // конец цикла поиска комбинаци диодов

//——————————————————————————————————————————————————————————————————————————————
//поиск новых значений напряжений и запись новых начальных данных для расчёта на
//следующем временном шаге (заряд конденсатора, ток через индуктивность, ...)
for (i = 0; i < Rebro_kol; i++)    //перебор по всем рёбрам
{
   ier = Class_Rebro[i]->New_UI(vector_F[i], dt_mks);
}

//——————————————————————————————————————————————————————————————————————————————
//печать результатов расчёта (напряжения и токи)
//ier = Print_current_data2(f[2],f[7], Uzel_kol, Rebro_kol, t, Class_Uzel[0], Class_Rebro, vector_F);

if(t>0.05)
{
	if(U_max_vah < std::fabs((double)Class_Rebro[1]->U))  U_max_vah = std::fabs((double)Class_Rebro[1]->U);
	if(I_max_vah < std::fabs((double)Class_Rebro[1]->I))  I_max_vah = std::fabs((double)Class_Rebro[1]->I);
}
//——————————————————————————————————————————————————————————————————————————————
t = t + dt_mks;  //время нового времянного слоя
pr_slojov++;     //инкримент количества временных шагов
if (pr_slojov%100==0) std::cout << pr_slojov;
}

fprintf(f[8],"%7.10f \t %7.10f \t %7.10f \n",(double)Class_Rebro[1]->R_rebra_ini+1/(double)Class_Rebro[1]->C_rebra_nonst/1000/2/3.14, U_max_vah, I_max_vah );
}

//——————————————————————————————————————————————————————————————————————————————
//Закрытие файлов
for(i=0; i < NF; i++) fclose(f[i]);

printf("%d",ier);   //вывод номера ошибки в консоль перед закрытием
return 0;
} //Конец програмы main






