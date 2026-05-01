from flask import Flask, request

app = Flask(__name__)
precios = { "mesa": "123.5", "reserva": "12.5" }

@app.route('/quitar-espacios', methods=["POST"])
def simple_espacio():
    try:
       req  = request.get_json()
       # Aqui va el string al que quieres quitar los espacios
       item = req['cadena']

       # Quitamos los espacios, devuelve las palabras en un array

       palabras_sin_espacios = item.split()
       item = " ".join(palabras_sin_espacios)
       return item, 200
    
    except Exception as e:
       return {"error": str(e)}, 415

app.run(debug=False, host="127.0.0.1", port="3000")