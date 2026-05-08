from email import message
from enum import Enum
from tkinter.messagebox import OK
from servicio_web_final.espacios_clnt import proxy_espacio_unico
import argparse
import socket
import threading
import errno


class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _socket_recepcion = None
    _finalizar_thread = 0
    _usuarios_conectados_datos = {}

    # Variable global para controlar el cliente que esta conectado
    _current_client = None

    # ******************** METHODS *******************
    # *
    # * @param user - User name to register in the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def  register(user) :
        #  Write your code here
        sd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)


        try:

            # Nos conectamos con los datos dados al servidor
            sd.connect((client._server,client._port))
            # Enviamos la instruccion de register
            message = b'REGISTER\0'
            sd.sendall(message)

            # Enviamos el nombre de usuario
            message = user.encode() + b'\0'
            sd.sendall(message)

            # Esperamos a obtener el código de ejecución
            respuesta = sd.recv(1)

            if(len(respuesta)<= 0):
                # La info no se recibió bien
                print("REGISTER FAIL")
                return client.RC.ERROR

            codigo= respuesta[0]
            if(codigo == 0):
                client._current_client = user
                print("REGISTER OK")
                return client.RC.OK

            elif(codigo == 1):
                print("USERNAME IN USE")
                return client.RC.USER_ERROR
            
            elif(codigo == 2):
                print("REGISTER FAIL")
                return client.RC.ERROR

        except:
            print("REGISTER FAIL")
            return client.RC.ERROR
        
        finally:
            # Cerramos el socket del cliente
            sd.close()

        return client.RC.ERROR

    # *
    # 	 * @param user - User name to unregister from the system
    # 	 * 
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred
    @staticmethod
    def  unregister(user) :
        #  Write your code here

        sd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

        try:
            # Nos conectamos con los datos dados al servidor
            sd.connect((client._server,client._port))
            # Enviamos la instruccion de register
            message = b'UNREGISTER\0'
            sd.sendall(message)

            # Enviamos el nombre de usuario
            message = user.encode() + b'\0'
            sd.sendall(message)

            # Esperamos a obtener el código de ejecución
            respuesta = sd.recv(1)

            if(len(respuesta)<= 0):
                # La info no se recibió bien
                print("UNREGISTER FAIL")
                return client.RC.ERROR

            codigo= respuesta[0]
            if(codigo == 0):
                client._current_client = None
                print("UNREGISTER OK")
                return client.RC.OK

            elif(codigo == 1):
                print("USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            
            elif(codigo == 2):
                print("UNREGISTER FAIL")
                return client.RC.ERROR

        except:
            print("UNREGISTER FAIL")
            return client.RC.ERROR
        
        finally:
            # Cerramos el socket del cliente
            sd.close()
        return client.RC.ERROR


    # *
    # * @param user - User name to connect to the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  connect(user) :
        #  Write your code here
        socket_envio_peticion = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

        socket_recepcion_mensajes = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        socket_recepcion_mensajes.bind(("0.0.0.0",0))
        socket_recepcion_mensajes.listen(1)
        puerto = socket_recepcion_mensajes.getsockname()[1]

        # Crear un hilo para recibir mensajes
        client._finalizar_thread = 0
        thread_recibo_mensajes = threading.Thread(target=client.worker,args = (socket_recepcion_mensajes,))
        thread_recibo_mensajes.daemon = True
        thread_recibo_mensajes.start()

        try:
            # Nos conectamos con los datos dados al servidor
            socket_envio_peticion.connect((client._server,client._port))
            # Enviamos la instruccion de register
            message = b'CONNECT\0'
            socket_envio_peticion.sendall(message)

            # Enviamos el nombre de usuario
            message = user.encode() + b'\0'
            socket_envio_peticion.sendall(message)

            # Enviamos el puerto asociado
            message = str(puerto).encode() + b'\0'
            socket_envio_peticion.sendall(message)

            respuesta = socket_envio_peticion.recv(1)

            if(len(respuesta)<= 0):
                # La info no se recibió bien
                socket_recepcion_mensajes.close()
                print("CONNECT FAIL")
                return client.RC.ERROR

            codigo= respuesta[0]
            
            if(codigo == 0):
                print("CONNECT OK")
                client._socket_recepcion = socket_recepcion_mensajes
                client._finalizar_thread = 0
                client._current_client = user
                return client.RC.OK

            elif(codigo == 1):
                socket_recepcion_mensajes.close()
                print("CONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            
            elif(codigo == 2):
                socket_recepcion_mensajes.close()
                print("USER ALREADY CONNECTED")
                return client.RC.USER_ERROR
            
            elif(codigo == 3):
                socket_recepcion_mensajes.close()
                print("CONNECT FAIL")
                return client.RC.ERROR

        
        except:
            socket_recepcion_mensajes.close()
            print("CONNECT FAIL")
            return client.RC.ERROR
        
        finally:
            socket_envio_peticion.close()

            
        return client.RC.ERROR

    # *
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  users() :
        #  Write your code here
        socket_envio_peticion = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        try:
            socket_envio_peticion.connect((client._server, client._port))
            message = b'USERS\0'
            socket_envio_peticion.sendall(message)
            message = client._current_client.encode() + b'\0'
            socket_envio_peticion.sendall(message)
            respuesta = socket_envio_peticion.recv(1)
            if(len(respuesta) <= 0):
                # La info no se recibió bien
                print("CONNECTED USERS FAIL")
                return client.RC.ERROR
            codigo = respuesta[0]

            if (codigo == 2):
                print("CONNECTED USERS FAIL")
                return client.RC.ERROR

            elif (codigo == 1):
                print("CONNECTED USERS FAIL, USER IS NOT CONNECTED")
                return client.RC.USER_ERROR
            
            elif (codigo == 0):
                respuesta_2 = socket_envio_peticion.recv(4)
                respuesta_2 = respuesta_2.rstrip(b'\0')
                if (len(respuesta_2) <= 0):
                    # La info no se recibió bien
                    print("CONNECTED USERS FAIL")
                    return client.RC.ERROR
                # Vamos a poner como límite 999 usuarios y lo que se recibe es una cadena de texto
                num_usuarios = int(respuesta_2.decode())
                num_procesados = 0
                buffer = b''
                print(f"CONNECTED USERS ({num_usuarios} users connected) OK")
                while num_procesados < num_usuarios:
                    respuesta_3 = socket_envio_peticion.recv(1024)
                    if len(respuesta_3) <= 0:
                        print("CONNECTED USERS FAIL")
                        return client.RC.ERROR
                    buffer += respuesta_3
                    while b'\0' in buffer:
                        user, buffer = buffer.split(b'\0', 1)
                        print(f"{user.decode()}")
                        # Guardamos los datos de los usuarios en una lista
                        # utilizando el deliminator ::
                        datos_usuario = user.decode().split("::")
                        # Lo guardamos en un diccionario con key nombre de usuario y valor una tupla (ip, puerto)
                        client._usuarios_conectados_datos[datos_usuario[0]] = (datos_usuario[1], int(datos_usuario[2]))
                        num_procesados += 1

                return client.RC.OK
        except:
            print("CONNECTED USERS FAIL")
            return client.RC.ERROR
        
        finally:
            socket_envio_peticion.close()

        return client.RC.ERROR


      # *
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  users_getfile() :
        socket_envio_peticion = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        try:
            socket_envio_peticion.connect((client._server, client._port))
            message = b'USERS\0'
            socket_envio_peticion.sendall(message)
            message = client._current_client.encode() + b'\0'
            socket_envio_peticion.sendall(message)
            respuesta = socket_envio_peticion.recv(1)
            if(len(respuesta) <= 0):
                # La info no se recibió bien
                return client.RC.ERROR
            codigo = respuesta[0]

            if (codigo == 2):
                return client.RC.ERROR

            elif (codigo == 1):
                return client.RC.USER_ERROR
            
            elif (codigo == 0):
                respuesta_2 = socket_envio_peticion.recv(4)
                respuesta_2 = respuesta_2.rstrip(b'\0')
                if (len(respuesta_2) <= 0):
                    # La info no se recibió bien
                    return client.RC.ERROR
                # Vamos a poner como límite 999 usuarios y lo que se recibe es una cadena de texto
                num_usuarios = int(respuesta_2.decode())
                num_procesados = 0
                buffer = b''
                while num_procesados < num_usuarios:
                    respuesta_3 = socket_envio_peticion.recv(1024)
                    if len(respuesta_3) <= 0:
                        return client.RC.ERROR
                    buffer += respuesta_3
                    while b'\0' in buffer:
                        user, buffer = buffer.split(b'\0', 1)
                        print(f"{user.decode()}")
                        # Guardamos los datos de los usuarios en una lista
                        # utilizando el deliminator ::
                        datos_usuario = user.decode().split("::")
                        # Lo guardamos en un diccionario con key nombre de usuario y valor una tupla (ip, puerto)
                        client._usuarios_conectados_datos[datos_usuario[0]] = (datos_usuario[1], int(datos_usuario[2]))
                        num_procesados += 1

                return client.RC.OK
        except:
            return client.RC.ERROR
        
        finally:
            socket_envio_peticion.close()

    
    @staticmethod
    def worker(socket_recepcion_mensajes):
        # Código que recibe los mensajes procedentes de otros clientes
        # socket_recepcion_mensajes contiene el socket que permite obtener dichos mensajes
        socket_recepcion_mensajes.settimeout(1.0)
        while(client._finalizar_thread == 0):
            try:
                while(1):
                    conexion_entrante, _ = socket_recepcion_mensajes.accept()
                    mensaje = conexion_entrante.recv(24)
                    mensaje = mensaje.rstrip(b'\0')
                    if mensaje.decode() == 'SEND_MESS_ACK':
                        id = conexion_entrante.recv(4)
                        id = id.rstrip(b'\0')
                        print(f"SEND MESSAGE {int(id.decode())} OK")
                    
                    elif mensaje.decode() == 'SEND_MESSAGE_ATTACH_ACK':
                        id = conexion_entrante.recv(4)
                        id = id.rstrip(b'\0')
                        print(f"SEND MESSAGE {int(id.decode())} OK")
                        
                    elif mensaje.decode() == 'SEND_MESSAGE':
                        elementos = []
                        elementos_recibidos = 0
                        while elementos_recibidos < 3:
                            # El mensaje se recibe como 1285 porque sería el máximo que se puede recibir teniendo en cuenta el tamaño máximo del mensaje (255 + 1) y el nombre de usuario (1024 + 1) y el id (3 + 1)
                            message = conexion_entrante.recv(1285)
                            while b'\0' in message:
                                # Recibe nombre, id y mensaje en ese orden separados por \0
                                mensaje_recibido, message = message.split(b'\0', 1)
                                elementos.append(mensaje_recibido.decode())
                                elementos_recibidos += 1
                        print(f"MESSAGE {int(elementos[1])} FROM {elementos[0]}\n"
                            f"{elementos[2]}")
                        
                    elif mensaje.decode() == 'SEND_MESSAGE_ATTACH':
                        elementos = []
                        elementos_recibidos = 0
                        # En este caso recibimos 4 elementos, ya que añadimos el nombre del archivo
                        while elementos_recibidos < 4:
                            # Por ello se le añade a 1285 256, que sería 1541
                            message = conexion_entrante.recv(1541)
                            while b'\0' in message:
                                # Recibe nombre, id y mensaje en ese orden separados por \0
                                mensaje_recibido, message = message.split(b'\0', 1)
                                elementos.append(mensaje_recibido.decode())
                                elementos_recibidos += 1
                        print(f"MESSAGE {int(elementos[1])} FROM {elementos[0]}\n"
                            f"{elementos[2]}\n END \n FILE {elementos[3]}")
                    
                    elif mensaje.decode() == 'GET_FILE':
                        elementos = []
                        elementos_recibidos = 0
                        while elementos_recibidos < 2:
                            message = conexion_entrante.recv(512)
                            while b'\0' in message:
                                mensaje_recibido, message = message.split(b'\0', 1)
                                elementos.append(mensaje_recibido.decode())
                                elementos_recibidos += 1
                        print(f"FILE {elementos[1]} REQUESTED BY {elementos[0]}")
                        
                        try:
                            
                            with open(elementos[1], 'rb') as f:
                                conexion_entrante.sendall(b'0')
                                # Leemos el archivo en bloques de 1024 bytes
                                bloque = f.read(1024)
                                while bloque:
                                    conexion_entrante.sendall(bloque)
                                    bloque = f.read(1024)

                                f.close()
                            conexion_entrante.close()
                        

                        except FileNotFoundError as efnf:
                            print(f"FILE TRANSFER FAILED{efnf}")
                            conexion_entrante.sendall(b'1')
                            conexion_entrante.close()
                            continue
                            
                        except Exception as e:
                            print(f"FILE TRANSFER FAILED{e}")
                            conexion_entrante.sendall(b'2')
                            conexion_entrante.close()
                            continue


            except OSError as e:
                # El thread fue cerrado de forma inesperada, finalizamos la ejecución
                if e.errno == errno.EBADF:
                    break
                continue

            except socket.timeout:
                continue
            
        socket_recepcion_mensajes.close()


    # *
    # * @param user - User name to disconnect from the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def  disconnect(user) :
        #  Write your code here
        socket_envio_peticion = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        try:
            socket_envio_peticion.connect((client._server,client._port))
            message = b'DISCONNECT\0'
            socket_envio_peticion.sendall(message)
            message = user.encode() + b'\0'
            socket_envio_peticion.sendall(message)

            respuesta = socket_envio_peticion.recv(1)

            if(len(respuesta)<= 0):
                # La info no se recibió bien
                print("DISCONNECT FAIL")
                return client.RC.ERROR
            
            codigo = respuesta[0]
            if(codigo == 0):
                client._current_client = None
                print("DISCONNECT OK")
                client._finalizar_thread = 1
                client._socket_recepcion.close()
                return client.RC.OK
            
            elif(codigo == 1):
                print("DISCONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            
            elif(codigo == 2):
                print("DISCONNECT FAIL, USER NOT CONNECTED")
                return client.RC.ERROR

            elif(codigo == 3):
                print("DISCONNECT FAIL")
                return client.RC.ERROR

        
        except:
            print("DISCONNECT FAIL")
            return client.RC.ERROR
        finally:
            socket_envio_peticion.close()





        return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  send(user,  message) :
                #  Write your code here
        socket_envio_peticion = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

        try:
            socket_envio_peticion.connect((client._server,client._port))
            mensaje = b'SEND\0'
            socket_envio_peticion.sendall(mensaje)
            mensaje = client._current_client.encode() + b'\0'
            socket_envio_peticion.sendall(mensaje)
            mensaje = user.encode() + b'\0'
            socket_envio_peticion.sendall(mensaje)
            # Como mucho el message tiene 256 caracteres
            # Obtenemos el mensaje con un único espacio entre palabras
            message = proxy_espacio_unico(message)

            if(message == None):
                print("SEND FAIL")
                return client.RC.ERROR
            
            mensaje = message.encode() + b'\0'
            if len(mensaje) > 255:
                print("SEND FAIL")
                return client.RC.ERROR
            socket_envio_peticion.sendall(mensaje)

            respuesta = socket_envio_peticion.recv(1)

            if(len(respuesta)<= 0):
                # La info no se recibió bien
                print("SEND FAIL")
                return client.RC.ERROR
            
            codigo = respuesta[0]
            if(codigo == 0):
                respuesta_2 = socket_envio_peticion.recv(4)
                respuesta_2 = respuesta_2.rstrip(b'\0')
                if (len(respuesta_2) <= 0):
                    # La info no se recibió bien
                    print("SEND FAIL")
                    return client.RC.ERROR
                # Vamos a poner como límite 999 usuarios y lo que se recibe es una cadena de texto
                id = int(respuesta_2.decode())
                print(f"SEND OK - MESSAGE {id}")
                # El ACK se recibirá en el worker en background
                return client.RC.OK
            
            elif(codigo == 1):
                print("SEND FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            
            elif(codigo == 2):
                print("SEND FAIL")
                return client.RC.ERROR
        
        except Exception as e:
            print(f"SEND FAIL{e}")
            return client.RC.ERROR
        finally:
            socket_envio_peticion.close()
        return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param file    - file  to be sent
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  sendAttach(user,  file,  message) :
                #  Write your code here
        socket_envio_peticion = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

        try:
            socket_envio_peticion.connect((client._server,client._port))
            mensaje = b'SENDATTACH\0'
            socket_envio_peticion.sendall(mensaje)
            mensaje = client._current_client.encode() + b'\0'
            socket_envio_peticion.sendall(mensaje)
            mensaje = user.encode() + b'\0'
            socket_envio_peticion.sendall(mensaje)
            # Como mucho el message tiene 256 caracteres
            # Obtenemos el mensaje con un único espacio entre palabras
            message = proxy_espacio_unico(message)

            if(message == None):
                print("SEND FAIL")
                return client.RC.ERROR
            
            mensaje = message.encode() + b'\0'
            if len(mensaje) > 255:
                print("SENDATTACH FAIL")
                return client.RC.ERROR
            socket_envio_peticion.sendall(mensaje)

            mensaje = file.encode() + b'\0'
            if len(mensaje) > 255:
                print("SENDATTACH FAIL")
                return client.RC.ERROR
            socket_envio_peticion.sendall(mensaje)

            respuesta = socket_envio_peticion.recv(1)

            if(len(respuesta)<= 0):
                # La info no se recibió bien
                print("SENDATTACH FAIL")
                return client.RC.ERROR
            
            codigo = respuesta[0]
            if(codigo == 0):
                respuesta_2 = socket_envio_peticion.recv(4)
                respuesta_2 = respuesta_2.rstrip(b'\0')
                if (len(respuesta_2) <= 0):
                    # La info no se recibió bien
                    print("SENDATTACH FAIL")
                    return client.RC.ERROR
                # Vamos a poner como límite 999 usuarios y lo que se recibe es una cadena de texto
                id = int(respuesta_2.decode())
                print(f"SENDATTACH OK - MESSAGE {id}")
                # El ACK se recibirá en el worker en background
                return client.RC.OK
            
            elif(codigo == 1):
                print("SENDATTACH FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            
            elif(codigo == 2):
                print("SENDATTACH FAIL")
                return client.RC.ERROR
        
        except Exception as e:
            print(f"SENDATTACH FAIL{e}")
            return client.RC.ERROR
        finally:
            socket_envio_peticion.close()
        return client.RC.ERROR
    
    @staticmethod
    def  getFile(userName, fileName, localFileName) :
        if(userName not in client._usuarios_conectados_datos.keys()):
            # Como no tenemos la información de ese user, probamos una petición de users
            client.users_getfile()

            # Una vez obtenidos los nuevos usuarios conectados, comprobamos si el usuario deseado está en el diccionario
            if(userName not in client._usuarios_conectados_datos.keys()):
                # El usuario no está conectado
                print("FILE TRANSFER FAIL, user not connected")
                return client.RC.USER_ERROR
            
        # Tenemos los datos del usuario, procedemos a realizar la petición
        socket_envio_peticion = socket.socket(socket.AF_INET,socket.SOCK_STREAM)

        try:
            ip = client._usuarios_conectados_datos[userName][0]
            puerto = client._usuarios_conectados_datos[userName][1]
            socket_envio_peticion.connect((ip, puerto))

            # Nos aseguramos que enviamos 24 bytes
            mensaje = b'GET_FILE' + b'\0' * 16
            socket_envio_peticion.sendall(mensaje)
            mensaje = client._current_client.encode() + b'\0'
            socket_envio_peticion.sendall(mensaje)
            mensaje = fileName.encode() + b'\0'
            socket_envio_peticion.sendall(mensaje)
            

            # Vamos a leer el archivo en bloques de 1024 bytes
            respuesta = int(socket_envio_peticion.recv(1).decode())
            if(respuesta ==  0):

                with open(localFileName, 'wb') as f:

                    contenido_recibido = socket_envio_peticion.recv(1024)
                    while  contenido_recibido:
                        f.write(contenido_recibido)
                        # Procesamos el siguiente bloque
                        contenido_recibido = socket_envio_peticion.recv(1024)
                    # Cerramos el archivo
                    f.close()
                print("FILE TRANSFER OK")
                return client.RC.OK
            
            elif(respuesta == 1):
                print("FILE TRANSFER FAIL, specified fileName does not exist")
                return client.RC.USER_ERROR
            
            elif(respuesta == 2):
                print("FILE TRANSFER FAIL, system failed")
                return client.RC.ERROR
             
        except  Exception as e:
            print(f"FILE TRANSFER FAILED{e}")
            return client.RC.ERROR
        
        finally:
            socket_envio_peticion.close()
        return client.RC.ERROR



    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="USERS") :
                        if (len(line) == 1) :
                            client.users()
                        else :
                            print("Syntax error. Usage: CONNECTED_USERS <userName>")

                    elif(line[0]=="SEND") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            message = ' '.join(line[2:])
                            client.send(line[1], message)
                        else :
                            print("Syntax error. Usage: SEND <userName> <message>")

                    elif(line[0]=="SENDATTACH") :
                        if (len(line) >= 4) :
                            #  Remove first two words
                            filename = line[-1]
                            message = ' '.join(line[2:-1])
                            client.sendAttach(line[1], filename, message)
                        else :
                            print("Syntax error. Usage: SENDATTACH <userName> <message> <filename>")
                    
                    elif(line[0] == "GETFILE"):
                        if(len(line) == 4):
                            client.getFile(line[1], line[2], line[3])
                        else:
                            print("Syntax error. Usage: GETFILE <userName> <fileName> <localFileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False
        
        client._server = args.s
        client._port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        # RECORDAR QUE HAY QUE CREAR UN THREAD PARA ENVIAR MENSAJES AL SERVIDOR Y OTRO QUE SE ENCARGARA
        # DE RECIBIR MENSAJES DEL SERVER PROCEDENTES DE OTROS CLIENTES
        # El que envia mensajes es el principal y el que recibe será el otro que se crea al realizar el connect
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])
