# Cada usuario va a tener su propio archivo. 
El archivo está formado por un struct inicial con los datos del usuario y después struct mensajes.

Cuando se reciba un mensaje y el cliente no está conectado se añade al final del archivo. Si el cliente se conecta, se van leyendo los mensajes uno por uno y se intentar enviar. Los que no se consiguen enviar, se almacenan en un array de forma que después el archivo se reescribe con los datos del usuario de nuevo al principio y al final los mensajes que no se consiguieron enviar.


# Dudas

Preguntar si despues de enviar y recibir un mensaje tmb se debe mostrar c>.