using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Threading;

public class Encryption
{
    [DllImport(@"..\..\..\Debug\RotationDLL.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void chiffrer(StringBuilder buffer, int decalage);

    [DllImport(@"..\..\..\Debug\RotationDLL.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void dechiffrer(StringBuilder str, int decalage);
}

public class Functions
{
    //méthode qui sert à chiffrer un StringBuilder et qui retourne sa valeur chiffré sous forme de string
    public static string ChiffrerStringBuilder(StringBuilder buffer)
    {
        if (buffer.Length > 0)
        {
            Encryption.chiffrer(buffer, 5);
            return buffer.ToString();
        }
        return "";

    }


    //Méthode servant à faire des paquets avec des données
    public static List<string> MakePaquets(string data)
    {
        //On prépare une liste de paquet
        List<string> paquets = new List<string>();
        //On ajoute un permier paquet, pour que la taille en bytes de ce qui reste à envoyer soit un multiple de 512
        paquets.Add(data.Substring(0, data.Length % 512));
        data = data.Substring(data.Length % 512, data.Length - data.Length % 512);
        //On prend ce qui reste à envoyer et on le sépare en paquets de 512 bytes
        for (int i = 0; i < data.Length / 512; i++)
        {
            if (data.Length >= 512 * (i + 1))
            {
                paquets.Add(data.Substring(512 * i, 512));
            }
        }
        //On retourne les paquets
        return paquets;
    }

    //Méthode servant à recevoir un message de la part du sender et à retourner le nombre de bytes reçues
    public static int ReceiveData(Socket sender, ref string message)
    {
        byte[] bytes = new byte[1024];
        int bytesRec = sender.Receive(bytes);
        message = Encoding.ASCII.GetString(bytes, 0, bytesRec);
        StringBuilder messageSB = new StringBuilder(message);
        Encryption.dechiffrer(messageSB, 5);
        message = messageSB.ToString();
        return bytesRec;
    }

    //Sert à envoyer un fichier au sender
    public static void SendFile(Socket sender, string fileName)
    {
        try
        {
            //On prépare les paquets à envoyer
            if (fileName == "") throw new IOException();
            List<string> paquets = MakePaquets(File.ReadAllText(fileName));
            int bytesSent = sender.Send(Encoding.ASCII.GetBytes(ChiffrerStringBuilder(new StringBuilder("sfile " + fileName))));
            string clientMsg = "";
            int byteRecv = 0;

            byteRecv = ReceiveData(sender, ref clientMsg);
            clientMsg = "";
            //On envoi un à un les paquets au sender
            foreach (string s in paquets)
            {
                bytesSent = sender.Send(Encoding.ASCII.GetBytes(ChiffrerStringBuilder(new StringBuilder(s))));
                while (clientMsg != "pr")
                {
                    byteRecv = ReceiveData(sender, ref clientMsg);
                }
                clientMsg = "";
            }
            //On envoi efile qui signale au sender que l'envoi du fichier est terminé
            bytesSent = sender.Send(Encoding.ASCII.GetBytes(ChiffrerStringBuilder(new StringBuilder("efile"))));
        }
        catch (IOException)
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"erreur, impossible de trouver ou de lire le fichier");
            Console.ForegroundColor = ConsoleColor.White;
        }
    }

   
    
}



public class SocketClient
{
    public static int Main(String[] args)
    {
        Console.ForegroundColor = ConsoleColor.Red;
        string serverIP = "127.0.0.1"; int port = 80;
        IPEndPoint remoteEP = new IPEndPoint(System.Net.IPAddress.Parse(serverIP), port);
        Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        try
        {
            socket.Connect(remoteEP);
            Console.WriteLine("Socket connectée à {0}", socket.RemoteEndPoint.ToString());

            string message = "";
            bool endOfCommunication = false;
            //Recevoir la confirmation de connexion 
            int bytes = Functions.ReceiveData(socket, ref message);
            if (bytes != 0)
            {
                
                Console.WriteLine("Je suis le client #" + message + "\n");
                Console.ForegroundColor = ConsoleColor.White;
                socket.Send(Encoding.ASCII.GetBytes(Functions.ChiffrerStringBuilder(new StringBuilder("ok"))));
                //Recevoir la confirmation qu'on a le droit d'écrire
                bytes = Functions.ReceiveData(socket, ref message);
                if(message == "ok")
                {
                    while (!endOfCommunication)
                    {
                        //Envoi de messages, tant qu'on est en communication
                        message = Console.ReadLine();
                        endOfCommunication = message == "fin";
                        if (message.Contains("sfile"))
                        {
                            string fileName = "";
                            try
                            {
                                fileName = message.Substring(6, message.Length - 6);

                            }
                            catch (Exception e)
                            {
                                fileName = "";
                            }
                            Functions.SendFile(socket, fileName);
                        }
                        else
                        {
                            socket.Send(Encoding.ASCII.GetBytes(Functions.ChiffrerStringBuilder(new StringBuilder(message))));
                        }
                        
                    }
                }
               
            }
        }
        catch (ArgumentNullException ane) { Console.WriteLine("ArgumentNullException : {0}", ane.ToString()); }
        catch (SocketException se) { Console.WriteLine("Erreur de communication avec le serveur"); }
        catch (Exception e) { Console.WriteLine("Unexpected exception : {0}", e.ToString()); }

        Console.ForegroundColor = ConsoleColor.Red;
        socket.Shutdown(SocketShutdown.Both);
        socket.Close();
        Console.WriteLine("Connection éteinte");
        return 0;   
    }


}
