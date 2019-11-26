#include"rotation.h"
#include<string>

extern "C" DLL_ALGO_ROTATION void chiffrer(char* buffer, int decalage)
{
	std::string str = buffer;

	//ON ramène le décalage à un nombre inférieur au nombre de lettre dans l'alphabet
	if (decalage >= ALPHA_COUNT) {
		decalage %= ALPHA_COUNT;
	}
	char c;

	
	for (int i = 0; i < str.length(); i++)
	{
		c = str[i];
		//Si le caractère courant est alphabétique
		if (c >= 'a' && c <= 'z')
		{
			//On prend la position dans le tableau ascii et on enlève la plus petite lettre alphabetique soit a
			//Cela fait en sorte qu'on se trouve avec une valeur correspondant à un nombre se situant entre 1 et 26
			c = c - 'a';
			if (c + decalage >= ALPHA_COUNT)
			{
				//Application du décalage - ALPHA_COUNT, car c + décalage et un nombre supérieur à 26, 
				//donc on reprend la boucle à partir de a
				c = c + decalage - ALPHA_COUNT;
			}
			else
			{
				//Application du décalage
				c = c + decalage;
			}
			//On remet le nouveau caractère décalé dans le string
			str[i] = c + 'a';
		}
		//Si le caractère courant est alphabétique (majuscule) (même logique que pour minuscule)
		else if (c >= 'A' && c <= 'Z')
		{
			c = c - 'A';
			if (c + decalage >= ALPHA_COUNT)
			{
				c = c + decalage - ALPHA_COUNT;
			}
			else
			{
				c = c + decalage;
			}
			str[i] = c + 'A';
		}

	}
	strcpy_s(buffer, strlen(buffer) + 1, str.c_str());
}

extern "C" DLL_ALGO_ROTATION void dechiffrer(char* buffer, int decalage)
{
	std::string str = buffer;

	//ON ramène le décalage à un nombre inférieur au nombre de lettre dans l'alphabet
	if (decalage >= ALPHA_COUNT) {
		decalage %= ALPHA_COUNT;
	}
	char c;
	for (int i = 0; i < str.length(); i++)
	{
		c = str[i];
		//Si le caractère courant est alphabétique
		if (c >= 'a' && c <= 'z')
		{
			//On prend la position dans le tableau ascii et on enlève la plus petite lettre alphabetique soit a
			//Cela fait en sorte qu'on se trouve avec une valeur correspondant à un nombre se situant entre 1 et 26
			c = c - 'a';
			if (c - decalage < 0)
			{
				//Application du décalage - ALPHA_COUNT, car c - décalage et un nombre inférieur à 0, 
				//donc on reprend la boucle à partir de a
				c = c - decalage + ALPHA_COUNT;
			}
			else
			{
				//Application du décalage
				c = c - decalage;
			}
			str[i] = c + 'a';
		}
		//Si le caractère courant est alphabétique (majuscule) (même logique que pour minuscule)
		else if (c >= 'A' && c <= 'Z')
		{
			c = c - 'A';
			if (c - decalage < 0)
			{
				c = c - decalage + ALPHA_COUNT;
			}
			else
			{
				c = c - decalage;
			}
			str[i] = c + 'A';
		}

	}
	strcpy_s(buffer, strlen(buffer) + 1, str.c_str());
}