#include"rotation.h"
#include<string>

extern "C" DLL_ALGO_ROTATION void chiffrer(char* buffer, int decalage)
{
	std::string str = buffer;

	//ON ram�ne le d�calage � un nombre inf�rieur au nombre de lettre dans l'alphabet
	if (decalage >= ALPHA_COUNT) {
		decalage %= ALPHA_COUNT;
	}
	char c;

	
	for (int i = 0; i < str.length(); i++)
	{
		c = str[i];
		//Si le caract�re courant est alphab�tique
		if (c >= 'a' && c <= 'z')
		{
			//On prend la position dans le tableau ascii et on enl�ve la plus petite lettre alphabetique soit a
			//Cela fait en sorte qu'on se trouve avec une valeur correspondant � un nombre se situant entre 1 et 26
			c = c - 'a';
			if (c + decalage >= ALPHA_COUNT)
			{
				//Application du d�calage - ALPHA_COUNT, car c + d�calage et un nombre sup�rieur � 26, 
				//donc on reprend la boucle � partir de a
				c = c + decalage - ALPHA_COUNT;
			}
			else
			{
				//Application du d�calage
				c = c + decalage;
			}
			//On remet le nouveau caract�re d�cal� dans le string
			str[i] = c + 'a';
		}
		//Si le caract�re courant est alphab�tique (majuscule) (m�me logique que pour minuscule)
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

	//ON ram�ne le d�calage � un nombre inf�rieur au nombre de lettre dans l'alphabet
	if (decalage >= ALPHA_COUNT) {
		decalage %= ALPHA_COUNT;
	}
	char c;
	for (int i = 0; i < str.length(); i++)
	{
		c = str[i];
		//Si le caract�re courant est alphab�tique
		if (c >= 'a' && c <= 'z')
		{
			//On prend la position dans le tableau ascii et on enl�ve la plus petite lettre alphabetique soit a
			//Cela fait en sorte qu'on se trouve avec une valeur correspondant � un nombre se situant entre 1 et 26
			c = c - 'a';
			if (c - decalage < 0)
			{
				//Application du d�calage - ALPHA_COUNT, car c - d�calage et un nombre inf�rieur � 0, 
				//donc on reprend la boucle � partir de a
				c = c - decalage + ALPHA_COUNT;
			}
			else
			{
				//Application du d�calage
				c = c - decalage;
			}
			str[i] = c + 'a';
		}
		//Si le caract�re courant est alphab�tique (majuscule) (m�me logique que pour minuscule)
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