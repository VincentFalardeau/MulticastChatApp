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
            //while(clientMsg != "pr")
            //{
                
            //}
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
            Console.WriteLine($"ERREUR, impossible de trouver ou de lire le fichier");
        }
    }

    //Sert à recevoir un fichier de la part du sender
    public static void ReceiveFile(Socket sender, string fileName)
    {
        try
        {
            //On ouvre un ofs au nom du fichier reçu
            StreamWriter ofs = new StreamWriter(fileName);
            int bytesSent = sender.Send(Encoding.ASCII.GetBytes(ChiffrerStringBuilder(new StringBuilder("pr"))));
            bytesSent = sender.Send(Encoding.ASCII.GetBytes(ChiffrerStringBuilder(new StringBuilder("pr"))));
            string clientMsg = "";
            int bytesRec = 0;
            //Tant que la réception du fichier n'est pas terminée
            while (clientMsg != "efile")
            {

                //Écriture au ofs
                ofs.Write(clientMsg);

                //Réception du prochain paquet
                bytesRec = ReceiveData(sender, ref clientMsg);
                
                if(clientMsg != "efile")
                {
                    //Envoi de la confirmation au sender
                    bytesSent = sender.Send(Encoding.ASCII.GetBytes(ChiffrerStringBuilder(new StringBuilder("pr"))));
                }
         
            }
            ofs.Close();
            Console.ForegroundColor = ConsoleColor.Blue;

        }
        catch (Exception)
        {
            Console.WriteLine("Erreur de réception du fichier");
        }
    }
}

//public class ThreadManipulator
//{
//    private Socket serverSocket;
//    private bool endOfCommunication;
//    private bool endOfServerCommunication;
//    //private bool fileReceiving;
//    private Thread threadRecv;
//    private bool fileSending;
//    public ThreadManipulator(Socket serverSocket)
//    {
//        this.serverSocket = serverSocket;
//        endOfCommunication = false;
//        endOfServerCommunication = false;
//        //fileReceiving = false;
//        fileSending = false;
//    }

//    public void StartCommunication()
//    {
//        threadRecv = new Thread(new ThreadStart(this.Receive));
//        Thread threadSend = new Thread(new ThreadStart(this.Send));
//        threadRecv.Start();
//        threadSend.Start();
//    }

//    public void Receive()
//    {
//        string clientMsg = "";
//        while (!endOfServerCommunication && !endOfCommunication)
//        {
//            try
//            {
//                Console.ForegroundColor = ConsoleColor.Blue;
//                Console.WriteLine(clientMsg);

//                Console.ForegroundColor = ConsoleColor.Green;
//                Functions.ReceiveData(serverSocket, ref clientMsg);
//                endOfServerCommunication = clientMsg == "fin";
//                if (clientMsg.Contains("sfile") && !fileSending)
//                {
//                    fileReceiving = true;
//                    string fileName = clientMsg.Substring(6, clientMsg.Length - 6);
//                    Functions.ReceiveFile(serverSocket, fileName);
//                    clientMsg = "Fichier reçu: " + fileName;
//                    fileReceiving = false;  

//                }
//            }
//            catch (Exception e)
//            {
//                break;
//            }

//        }
//        CloseConnection();
//    }

//    public void Send()
//    {
//        string msgToClient = "";
//        while (!endOfServerCommunication && !endOfCommunication)
//        {
//            try
//            {
//                msgToClient = "";
//                Console.ForegroundColor = ConsoleColor.Green;
//                msgToClient = Console.ReadLine();
//                //while (FileReceiving) { }
//                endOfCommunication = msgToClient == "fin";
//                if (msgToClient.Contains("sfile"))
//                {
//                    fileSending= true;
//                    string fileName = "";
//                    try
//                    {
//                         fileName = msgToClient.Substring(6, msgToClient.Length - 6);
                       
//                    }
//                    catch (Exception e)
//                    {
//                        fileName = "";
//                    }
//                    threadRecv.Suspend();
//                    Functions.SendFile(serverSocket, fileName);
//                    threadRecv.Resume();

//                    fileSending = false;

//                }
//                else if(!fileReceiving)
//                {
//                    serverSocket.Send(Encoding.ASCII.GetBytes(Functions.ChiffrerStringBuilder(new StringBuilder(msgToClient))));
//                }
               
//            }
//            catch (Exception e)
//            {
//                break;
//            }

//        }
//        CloseConnection();
//    }

//    private void CloseConnection()
//    {
//        try
//        {
//            Console.ForegroundColor = ConsoleColor.Magenta;
//            serverSocket.Shutdown(SocketShutdown.Both);
//            serverSocket.Close();
//            Console.WriteLine("Connection éteinte");
//        }
//        catch (Exception e)
//        {
//        }

//    }
//}

public class SocketClient
{
    public static int Main(String[] args)
    {
        Console.ForegroundColor = ConsoleColor.Red;
        string serverIP = "127.0.0.1"; int port = 8080;
        IPEndPoint remoteEP = new IPEndPoint(System.Net.IPAddress.Parse(serverIP), port);
        Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        try
        {
            socket.Connect(remoteEP);
            Console.WriteLine("Socket connectée à {0}", socket.RemoteEndPoint.ToString());
            //ThreadManipulator monThread = new ThreadManipulator(sender);
            //monThread.StartCommunication();

            const string OK = "ok";
            const string END = "fin";

            string message = "";
            bool endOfCommunication = false;
            int bytes = Functions.ReceiveData(socket, ref message);
            if (bytes != 0)
            {
                //Attention, dans le substring, si le numéro du client comporte plus de 1 caractère, l'affichage sera erroné
                Console.WriteLine("Je suis le client #" + message.Substring(0,1) + "\n");
                //Au cas où le numéro du client et le message de confirmation sont envoyées en même temps
                if (!message.Contains(OK))
                {
                    Functions.ReceiveData(socket, ref message);
                }
                else
                {
                    message = OK;
                }

                if(message == OK)
                {
                    Console.ForegroundColor = ConsoleColor.White;
                    while (!endOfCommunication)
                    {
                        message = Console.ReadLine();
                        endOfCommunication = message == END;
                        socket.Send(Encoding.ASCII.GetBytes(Functions.ChiffrerStringBuilder(new StringBuilder(message))));
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
