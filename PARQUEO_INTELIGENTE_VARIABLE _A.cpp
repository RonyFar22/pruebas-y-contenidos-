
// Proyecto: Parqueo inteligente - Variante A
// Control de tarifas y ocupacion
//
// Estudiante:RONY FERNANDO FARFAN JOJ  Curso: ALGORITMOS 


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <cstdio>
using namespace std;

// variables 

const int MAX_ESPACIOS = 18;
const int MAX_HISTORIAL = 5000;
const int MIN_GRACIA = 10;            // minutos gratis
const char ARCHIVO[] = "parqueo.txt"; // aqui se guardan los datos

// 3 tipos de vehiculo
const int MOTO = 1;
const int CARRO = 2;
const int CAMION = 3;

// metodos de pago
const int SIN_PAGO = 0;
const int EFECTIVO = 1;
const int TARJETA = 2;

// Los arreglos usan el indice 1, 2 y 3 (el 0 no se usa)
const double TARIFA_HORA[4] = {0, 3, 5, 10};  // Q por hora
const double TARIFA_MAX[4] = {0, 20, 40, 80}; // tope por 24 horas
const int CANTIDAD[4] = {0, 5, 10, 3};        // espacios de cada tipo
const string NOMBRE_TIPO[4] = {"", "Moto", "Carro", "Camion"};


struct Espacio {
    int numero;
    int tipo;
    bool ocupado;
    string placa;  // vacia si el espacio esta libre
    long entrada;  // hora de entrada
};

struct Salida {  // un vehiculo que ya salio y pago
    string placa;
    int tipo;
    int espacio;
    long entrada;
    long salida;
    double cobro;
    bool perdido;
    int metodo;
    double recibido;
    double cambio;
    string last4;
    string aut;
};

struct Pago {
    int metodo;
    double recibido;
    string last4;
    string aut;
};

// variables generales 
Espacio espacios[MAX_ESPACIOS];
Salida historial[MAX_HISTORIAL];
int totalHistorial = 0;
long desfaseMinutos = 0;  // sirve para simular que pasa el tiempo


// Funciones de apoyo (tiempo, texto, dinero)


long ahora() {
    return (long)time(NULL) / 60 + desfaseMinutos;
}

string textoHora(long minutos) {
    time_t t = (time_t)(minutos * 60);
    char buf[32];
    strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", localtime(&t));
    return string(buf);
}

string textoFecha(long minutos) {
    time_t t = (time_t)(minutos * 60);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
    return string(buf);
}

string textoDuracion(long minutos) {
    ostringstream o;
    o << minutos / 60 << "h " << minutos % 60 << "m";
    return o.str();
}

string dinero(double valor) {
    ostringstream o;
    o << "Q" << fixed << setprecision(2) << valor;
    return o.str();
}

// quita los espacios y pasa a mayusculas
string aMayusculas(string texto) {
    string resultado = "";
    for (int i = 0; i < (int)texto.size(); i++) {
        if (texto[i] != ' ') {
            resultado += (char)toupper((unsigned char)texto[i]);
        }
    }
    return resultado;
}

bool placaValida(string placa) {
    if (placa.size() < 4 || placa.size() > 8) return false;
    for (int i = 0; i < (int)placa.size(); i++) {
        if (!isalnum((unsigned char)placa[i]) && placa[i] != '-') return false;
    }
    return true;
}

bool esNumero4(string texto) {
    if (texto.size() != 4) return false;
    for (int i = 0; i < 4; i++) {
        if (!isdigit((unsigned char)texto[i])) return false;
    }
    return true;
}

bool autorizacionValida(string texto) {
    if (texto.size() < 4 || texto.size() > 12) return false;
    for (int i = 0; i < (int)texto.size(); i++) {
        if (!isalnum((unsigned char)texto[i])) return false;
    }
    return true;
}

bool fechaValida(string f) {
    if (f.size() != 10) return false;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (f[i] != '-') return false;
        } else if (!isdigit((unsigned char)f[i])) {
            return false;
        }
    }
    return true;
}


// Lectura de datos 


string leerTexto(string mensaje) {
    string linea;
    cout << mensaje;
    if (!getline(cin, linea)) {
        cout << "\nSe termino la entrada de datos. Fin del programa.\n";
        exit(0);
    }
    if (linea.size() > 0 && linea[linea.size() - 1] == '\r') {
        linea.erase(linea.size() - 1);
    }
    return linea;
}

int leerEntero(string mensaje) {
    while (true) {
        string linea = leerTexto(mensaje);
        istringstream ss(linea);
        int n;
        char resto;
        if ((ss >> n) && !(ss >> resto)) {
            return n;
        }
        cout << "ERROR: escribe un numero entero.\n";
    }
}

// devuelve -1 si el texto no es un monto valido
double convertirMonto(string texto) {
    istringstream ss(texto);
    double x;
    char resto;
    if ((ss >> x) && !(ss >> resto) && x >= 0) {
        return x;
    }
    return -1;
}


//reglamentos que sigue el parqueo 


void iniciarEspacios() {
    int n = 0;
    for (int tipo = 1; tipo <= 3; tipo++) {
        for (int i = 0; i < CANTIDAD[tipo]; i++) {
            espacios[n].numero = n + 1;
            espacios[n].tipo = tipo;
            espacios[n].ocupado = false;
            espacios[n].placa = "";
            espacios[n].entrada = 0;
            n++;
        }
    }
    totalHistorial = 0;
}

Pago nuevoPago() {
    Pago p;
    p.metodo = SIN_PAGO;
    p.recibido = 0;
    p.last4 = "";
    p.aut = "";
    return p;
}

// Devuelve la posicion del vehiculo en el arreglo, o -1 si no esta adentro
int buscarPlaca(string placa) {
    for (int i = 0; i < MAX_ESPACIOS; i++) {
        if (espacios[i].ocupado && espacios[i].placa == placa) {
            return i;
        }
    }
    return -1;
}

// Devuelve la posicion de un espacio libre de ese tipo, o -1 si no hay
int buscarLibre(int tipo) {
    for (int i = 0; i < MAX_ESPACIOS; i++) {
        if (!espacios[i].ocupado && espacios[i].tipo == tipo) {
            return i;
        }
    }
    return -1;
}

// Cobro: 10 min de gracia, cada fraccion de hora cuenta como hora completa,
// y hay un tope por cada 24 horas.
double calcularCobro(int tipo, long minutos, bool perdido) {
    if (perdido) {
        // boleto perdido: se cobra el tope por cada dia iniciado
        long dias = (minutos + 1439) / 1440;
        if (dias < 1) dias = 1;
        return dias * TARIFA_MAX[tipo];
    }
    if (minutos <= MIN_GRACIA) {
        return 0;
    }
    long dias = minutos / 1440;      // dias completos
    long resto = minutos % 1440;     // minutos que sobran
    long horas = (resto + 59) / 60;  // se redondea hacia arriba
    double cobroResto = horas * TARIFA_HORA[tipo];
    if (cobroResto > TARIFA_MAX[tipo]) {
        cobroResto = TARIFA_MAX[tipo];
    }
    return dias * TARIFA_MAX[tipo] + cobroResto;
}

// Revisa que los datos de los espacios tengan sentido.
// Regla 1: un espacio ocupado tiene placa, uno libre no.
// Regla 2: la misma placa no puede estar en dos espacios.
bool verificarEstado() {
    for (int i = 0; i < MAX_ESPACIOS; i++) {
        if (espacios[i].numero != i + 1) return false;
        if (espacios[i].ocupado && espacios[i].placa == "") return false;
        if (!espacios[i].ocupado && espacios[i].placa != "") return false;
    }
    for (int i = 0; i < MAX_ESPACIOS; i++) {
        for (int j = i + 1; j < MAX_ESPACIOS; j++) {
            if (espacios[i].ocupado && espacios[j].ocupado &&
                espacios[i].placa == espacios[j].placa) {
                return false;
            }
        }
    }
    return true;
}

// con esto verificamos que los pagos sean corectos con respecto a los minutos o horas estacionadas 
bool verificarPagos() {
    for (int i = 0; i < totalHistorial; i++) {
        Salida s = historial[i];
        if (s.cobro == 0) {
            if (s.metodo != SIN_PAGO) return false;
        } else if (s.metodo == EFECTIVO) {
            double dif = s.recibido - s.cambio - s.cobro;
            if (dif > 0.005 || dif < -0.005) return false;
            if (s.cambio < 0) return false;
        } else if (s.metodo == TARJETA) {
            if (!esNumero4(s.last4) || !autorizacionValida(s.aut)) return false;
        } else {
            return false;
        }
    }
    return true;
}

bool registrarIngreso(string placa, int tipo, string &mensaje) {
    if (buscarPlaca(placa) != -1) {
        mensaje = "ERROR: el vehiculo " + placa + " ya tiene un ingreso activo.";
        return false;
    }
    int libre = buscarLibre(tipo);
    if (libre == -1) {
        mensaje = "ERROR: parqueo lleno para " + NOMBRE_TIPO[tipo] + ".";
        return false;
    }
    espacios[libre].ocupado = true;
    espacios[libre].placa = placa;
    espacios[libre].entrada = ahora();
    mensaje = "Ingreso registrado: " + placa + " -> espacio " +
              to_string(espacios[libre].numero) + " a las " +
              textoHora(espacios[libre].entrada) + ".";
    return true;
}

// Revisa que el pago alcance para el total. Si no, deja el motivo en mensaje.
bool validarPago(double total, Pago pago, string &mensaje) {
    if (total == 0) {
        return true;  // dentro de los 10 minutos no se cobrara 
    }
    if (pago.metodo == EFECTIVO) {
        if (pago.recibido < total) {
            mensaje = "ERROR: efectivo insuficiente, faltan " +
                      dinero(total - pago.recibido) + ".";
            return false;
        }
        return true;
    }
    if (pago.metodo == TARJETA) {
        if (!esNumero4(pago.last4)) {
            mensaje = "ERROR: tarjeta, se requieren exactamente 4 digitos.";
            return false;
        }
        if (!autorizacionValida(pago.aut)) {
            mensaje = "ERROR: tarjeta, numero de autorizacion invalido.";
            return false;
        }
        return true;
    }
    mensaje = "ERROR: falta el metodo de pago. El vehiculo no puede salir sin pagar.";
    return false;
}

string armarRecibo(Salida s) {
    ostringstream o;
    o << "------------------------------------\n";
    o << "        RECIBO DE PARQUEO\n";
    o << "------------------------------------\n";
    o << "Placa:         " << s.placa << "\n";
    o << "Tipo:          " << NOMBRE_TIPO[s.tipo] << "\n";
    o << "Espacio:       " << s.espacio << "\n";
    o << "Entrada:       " << textoHora(s.entrada) << "\n";
    o << "Salida:        " << textoHora(s.salida) << "\n";
    o << "Permanencia:   " << textoDuracion(s.salida - s.entrada) << "\n";
    if (s.perdido) {
        o << "Boleto:        PERDIDO (tope diario)\n";
    }
    o << "------------------------------------\n";
    o << "TOTAL A PAGAR: " << dinero(s.cobro) << "\n";
    if (s.metodo == EFECTIVO) {
        o << "Pago:          Efectivo\n";
        o << "Recibido:      " << dinero(s.recibido) << "\n";
        o << "Cambio:        " << dinero(s.cambio) << "\n";
    } else if (s.metodo == TARJETA) {
        o << "Pago:          Tarjeta **** " << s.last4 << "\n";
        o << "Autorizacion:  " << s.aut << "\n";
    } else {
        o << "Pago:          Sin cobro\n";
    }
    o << "------------------------------------\n";
    return o.str();
}

// La salida solo se hace si el pago es valido. Primero se revisa todo
// y hasta el final se libera el espacio.
bool registrarSalida(string placa, bool perdido, Pago pago, long horaSalida, string &mensaje) {
    int i = buscarPlaca(placa);
    if (i == -1) {
        mensaje = "ERROR: no existe ingreso activo para " + placa + ". No puede salir.";
        return false;
    }
    long minutos = horaSalida - espacios[i].entrada;
    if (minutos < 0) minutos = 0;
    double total = calcularCobro(espacios[i].tipo, minutos, perdido);

    if (total == 0) {
        pago = nuevoPago();
    } else if (!validarPago(total, pago, mensaje)) {
        return false;
    }
    if (totalHistorial >= MAX_HISTORIAL) {
        mensaje = "ERROR: el historial esta lleno.";
        return false;
    }

    // guardar en el historial
    Salida s;
    s.placa = placa;
    s.tipo = espacios[i].tipo;
    s.espacio = espacios[i].numero;
    s.entrada = espacios[i].entrada;
    s.salida = horaSalida;
    s.cobro = total;
    s.perdido = perdido;
    s.metodo = pago.metodo;
    s.recibido = 0;
    s.cambio = 0;
    s.last4 = "";
    s.aut = "";
    if (pago.metodo == EFECTIVO && total > 0) {
        s.recibido = pago.recibido;
        s.cambio = pago.recibido - total;
    }
    if (pago.metodo == TARJETA && total > 0) {
        s.last4 = pago.last4;
        s.aut = pago.aut;
    }
    historial[totalHistorial] = s;
    totalHistorial++;

    // liberar el espacio
    espacios[i].ocupado = false;
    espacios[i].placa = "";
    espacios[i].entrada = 0;

    mensaje = armarRecibo(s);
    return true;
}

// Calcula el total sin cambiar nada. Devuelve -1 si el vehiculo no esta adentro.
double cotizar(string placa, bool perdido, long horaSalida, long &minutos) {
    int i = buscarPlaca(placa);
    if (i == -1) return -1;
    minutos = horaSalida - espacios[i].entrada;
    if (minutos < 0) minutos = 0;
    return calcularCobro(espacios[i].tipo, minutos, perdido);
}

// Guardar y cargar datos en archivo

void guardarDatos() {
    ofstream f(ARCHIVO);
    if (!f) {
        cout << "AVISO: no se pudo guardar el archivo.\n";
        return;
    }
    f << fixed << setprecision(2);
    f << "PARQUEO 1\n";
    f << "OFFSET " << desfaseMinutos << "\n";
    for (int i = 0; i < MAX_ESPACIOS; i++) {
        if (espacios[i].ocupado) {
            f << "ACTIVO " << espacios[i].placa << " " << espacios[i].tipo << " "
              << espacios[i].numero << " " << espacios[i].entrada << "\n";
        }
    }
    for (int i = 0; i < totalHistorial; i++) {
        Salida s = historial[i];
        f << "HIST " << s.placa << " " << s.tipo << " " << s.espacio << " "
          << s.entrada << " " << s.salida << " " << s.cobro << " "
          << (s.perdido ? 1 : 0) << " " << s.metodo << " " << s.recibido << " "
          << s.cambio << " " << (s.last4 == "" ? "-" : s.last4) << " "
          << (s.aut == "" ? "-" : s.aut) << "\n";
    }
}

// Devuelve 0 si no hay archivo, 1 si cargo bien, -1 si el archivo esta danado
int cargarDatos() {
    ifstream f(ARCHIVO);
    if (!f) return 0;

    string linea;
    bool hayEncabezado = false;
    while (getline(f, linea)) {
        if (linea == "") continue;
        istringstream ss(linea);
        string clave;
        ss >> clave;

        if (clave == "PARQUEO") {
            hayEncabezado = true;
        } else if (!hayEncabezado) {
            return -1;
        } else if (clave == "OFFSET") {
            if (!(ss >> desfaseMinutos)) return -1;
        } else if (clave == "ACTIVO") {
            string placa;
            int tipo, numero;
            long entrada;
            if (!(ss >> placa >> tipo >> numero >> entrada)) return -1;
            if (tipo < 1 || tipo > 3 || numero < 1 || numero > MAX_ESPACIOS) return -1;
            int i = numero - 1;
            if (espacios[i].ocupado || espacios[i].tipo != tipo) return -1;
            espacios[i].ocupado = true;
            espacios[i].placa = placa;
            espacios[i].entrada = entrada;
        } else if (clave == "HIST") {
            Salida s;
            int perdido;
            string l4, au;
            if (!(ss >> s.placa >> s.tipo >> s.espacio >> s.entrada >> s.salida >> s.cobro
                     >> perdido >> s.metodo >> s.recibido >> s.cambio >> l4 >> au)) return -1;
            if (s.tipo < 1 || s.tipo > 3) return -1;
            if (s.espacio < 1 || s.espacio > MAX_ESPACIOS) return -1;
            if (s.metodo < 0 || s.metodo > 2) return -1;
            if (totalHistorial >= MAX_HISTORIAL) return -1;
            s.perdido = (perdido != 0);
            s.last4 = (l4 == "-" ? "" : l4);
            s.aut = (au == "-" ? "" : au);
            historial[totalHistorial] = s;
            totalHistorial++;
        } else {
            return -1;
        }
    }
    if (!hayEncabezado) return -1;
    if (!verificarEstado() || !verificarPagos()) return -1;
    return 1;
}

// ============================================================
// Pantallas y opciones del menu
// ============================================================

void mostrarMenu() {
    cout << "\n===== PARQUEO INTELIGENTE =====\n";
    cout << "1. Registrar ingreso\n";
    cout << "2. Registrar salida y cobrar\n";
    cout << "3. Consultar disponibilidad\n";
    cout << "4. Mapa de espacios\n";
    cout << "5. Consultar vehiculo\n";
    cout << "6. Ver tarifas\n";
    cout << "7. Cierre del dia\n";
    cout << "8. Simular paso del tiempo (minutos)\n";
    cout << "0. Salir\n";
}

void opcionIngreso() {
    string placa = aMayusculas(leerTexto("Placa: "));
    if (!placaValida(placa)) {
        cout << "ERROR: placa invalida (4 a 8 letras, numeros o guion).\n";
        return;
    }
    int tipo = leerEntero("Tipo (1=Moto, 2=Carro, 3=Camion): ");
    if (tipo < 1 || tipo > 3) {
        cout << "ERROR: tipo invalido.\n";
        return;
    }
    string mensaje;
    if (registrarIngreso(placa, tipo, mensaje)) {
        guardarDatos();
    }
    cout << mensaje << "\n";
}

// Pide el metodo de pago. Devuelve false si el operador cancela.
bool pedirPago(double total, Pago &pago) {
    cout << "Metodo de pago: 1 = Efectivo, 2 = Tarjeta, Enter = cancelar\n";
    string op = leerTexto("Metodo: ");

    if (op == "1") {
        pago.metodo = EFECTIVO;
        while (true) {
            string s = leerTexto("Monto recibido en efectivo (Enter = cancelar): Q");
            if (s == "") return false;
            double monto = convertirMonto(s);
            if (monto < 0) {
                cout << "ERROR: monto invalido (ejemplo: 50 o 50.00).\n";
            } else if (monto < total) {
                cout << "Monto insuficiente: faltan " << dinero(total - monto) << ".\n";
            } else {
                pago.recibido = monto;
                cout << "Cambio a entregar: " << dinero(monto - total) << "\n";
                return true;
            }
        }
    }
    if (op == "2") {
        pago.metodo = TARJETA;
        cout << "(Solo se piden los ultimos 4 digitos; el numero completo no se guarda.)\n";
        while (true) {
            string s = leerTexto("Ultimos 4 digitos (Enter = cancelar): ");
            if (s == "") return false;
            if (esNumero4(s)) {
                pago.last4 = s;
                break;
            }
            cout << "ERROR: deben ser exactamente 4 digitos.\n";
        }
        while (true) {
            string s = aMayusculas(leerTexto("No. de autorizacion del POS (Enter = cancelar): "));
            if (s == "") return false;
            if (autorizacionValida(s)) {
                pago.aut = s;
                break;
            }
            cout << "ERROR: autorizacion invalida (4 a 12 letras o numeros).\n";
        }
        return true;
    }
    return false;
}

void opcionSalida() {
    string placa = aMayusculas(leerTexto("Placa: "));
    string resp = leerTexto("Boleto perdido? (s/n): ");
    bool perdido = (resp == "s" || resp == "S");

    // 1) calcular cuanto se debe cobrar (todavia no se cambia nada)
    long hora = ahora();
    long minutos = 0;
    double total = cotizar(placa, perdido, hora, minutos);
    if (total < 0) {
        cout << "ERROR: no existe ingreso activo para " << placa << ". No puede salir.\n";
        return;
    }
    cout << "Permanencia: " << textoDuracion(minutos) << " | Total a cobrar: " << dinero(total) << "\n";

    // 2) pedir el pago
    Pago pago = nuevoPago();
    if (total > 0) {
        if (!pedirPago(total, pago)) {
            cout << "Pago cancelado. El vehiculo sigue dentro del parqueo.\n";
            return;
        }
    }

    // 3) hacer la salida
    string mensaje;
    if (registrarSalida(placa, perdido, pago, hora, mensaje)) {
        guardarDatos();
    }
    cout << mensaje << "\n";
}

void opcionDisponibilidad() {
    cout << "\n--- Disponibilidad ---\n";
    cout << left << setw(10) << "Tipo" << setw(10) << "Libres"
         << setw(10) << "Ocupados" << setw(8) << "Total" << "\n";
    int totalLibres = 0;
    int totalOcupados = 0;
    for (int tipo = 1; tipo <= 3; tipo++) {
        int libres = 0;
        int ocupados = 0;
        for (int i = 0; i < MAX_ESPACIOS; i++) {
            if (espacios[i].tipo == tipo) {
                if (espacios[i].ocupado) ocupados++;
                else libres++;
            }
        }
        cout << left << setw(10) << NOMBRE_TIPO[tipo] << setw(10) << libres
             << setw(10) << ocupados << setw(8) << libres + ocupados << "\n";
        totalLibres += libres;
        totalOcupados += ocupados;
    }
    cout << left << setw(10) << "TOTAL" << setw(10) << totalLibres
         << setw(10) << totalOcupados << setw(8) << totalLibres + totalOcupados << "\n";
}

void opcionMapa() {
    cout << "\n--- Mapa de espacios ---\n";
    for (int i = 0; i < MAX_ESPACIOS; i++) {
        cout << "[" << setw(2) << espacios[i].numero << " " << NOMBRE_TIPO[espacios[i].tipo][0] << "] ";
        if (espacios[i].ocupado) {
            cout << "OCUPADO " << espacios[i].placa << " desde " << textoHora(espacios[i].entrada);
        } else {
            cout << "libre";
        }
        cout << "\n";
    }
}

void opcionConsultar() {
    string placa = aMayusculas(leerTexto("Placa: "));
    int i = buscarPlaca(placa);
    if (i == -1) {
        cout << "No hay ingreso activo para " << placa << ".\n";
        return;
    }
    long minutos = ahora() - espacios[i].entrada;
    if (minutos < 0) minutos = 0;
    cout << placa << " en espacio " << espacios[i].numero << " desde " << textoHora(espacios[i].entrada)
         << " | permanencia " << textoDuracion(minutos)
         << " | cobro actual " << dinero(calcularCobro(espacios[i].tipo, minutos, false)) << "\n";
}

void opcionTarifas() {
    cout << "\n--- Tarifas (gracia " << MIN_GRACIA << " min, fraccion = hora completa) ---\n";
    for (int tipo = 1; tipo <= 3; tipo++) {
        cout << left << setw(8) << NOMBRE_TIPO[tipo] << dinero(TARIFA_HORA[tipo]) << "/hora, tope "
             << dinero(TARIFA_MAX[tipo]) << " por 24 h, " << CANTIDAD[tipo] << " espacios\n";
    }
    cout << "Boleto perdido: se cobra el tope por cada dia iniciado.\n";
    cout << "Pago: efectivo (con cambio) o tarjeta (4 digitos + autorizacion).\n";
}

void opcionCierre() {
    string fecha = leerTexto("Fecha (AAAA-MM-DD, Enter = hoy): ");
    if (fecha == "") {
        fecha = textoFecha(ahora());
    } else if (!fechaValida(fecha)) {
        cout << "ERROR: formato de fecha invalido.\n";
        return;
    }

    ostringstream o;
    o << "========== CIERRE DEL DIA " << fecha << " ==========\n";
    int cantidad = 0;
    double total = 0;
    int cantTipo[4] = {0, 0, 0, 0};
    double sumaTipo[4] = {0, 0, 0, 0};
    int nEfectivo = 0, nTarjeta = 0, nSinCobro = 0;
    double sumaEfectivo = 0, sumaTarjeta = 0;

    for (int i = 0; i < totalHistorial; i++) {
        Salida s = historial[i];
        if (textoFecha(s.salida) != fecha) continue;
        cantidad++;
        total += s.cobro;
        cantTipo[s.tipo]++;
        sumaTipo[s.tipo] += s.cobro;
        o << left << setw(9) << s.placa << setw(8) << NOMBRE_TIPO[s.tipo]
          << "esp " << setw(3) << s.espacio << setw(10) << textoDuracion(s.salida - s.entrada)
          << dinero(s.cobro);
        if (s.perdido) o << " [boleto perdido]";
        if (s.metodo == EFECTIVO) {
            o << "  Efectivo";
            nEfectivo++;
            sumaEfectivo += s.cobro;
        } else if (s.metodo == TARJETA) {
            o << "  Tarjeta ****" << s.last4 << " aut " << s.aut;
            nTarjeta++;
            sumaTarjeta += s.cobro;
        } else {
            o << "  Sin cobro";
            nSinCobro++;
        }
        o << "\n";
    }
    if (cantidad == 0) {
        o << "Sin salidas registradas ese dia.\n";
    }
    o << "------------------------------------\n";
    for (int tipo = 1; tipo <= 3; tipo++) {
        if (cantTipo[tipo] > 0) {
            o << left << setw(8) << NOMBRE_TIPO[tipo] << cantTipo[tipo] << " salidas, "
              << dinero(sumaTipo[tipo]) << "\n";
        }
    }
    o << "Vehiculos atendidos: " << cantidad << "\n";
    o << "INGRESOS TOTALES:    " << dinero(total) << "\n";
    o << "Por metodo de pago:\n";
    o << "  Efectivo: " << nEfectivo << " pago(s), " << dinero(sumaEfectivo) << "\n";
    o << "  Tarjeta:  " << nTarjeta << " pago(s), " << dinero(sumaTarjeta) << "\n";
    o << "  Sin cobro (gracia): " << nSinCobro << "\n";

    if (fecha == textoFecha(ahora())) {
        int dentro = 0;
        for (int i = 0; i < MAX_ESPACIOS; i++) {
            if (espacios[i].ocupado) dentro++;
        }
        o << "Aun dentro del parqueo: " << dentro << "\n";
        for (int i = 0; i < MAX_ESPACIOS; i++) {
            if (espacios[i].ocupado) {
                long minutos = ahora() - espacios[i].entrada;
                o << "  - " << espacios[i].placa << " (espacio " << espacios[i].numero
                  << ", cobro pendiente "
                  << dinero(calcularCobro(espacios[i].tipo, minutos, false)) << ")\n";
            }
        }
    }
    o << "Estado del sistema: " << ((verificarEstado() && verificarPagos()) ? "OK" : "FALLO") << "\n";
    o << "====================================\n";

    cout << "\n" << o.str();

    // guardar el cierre en un archivo
    string nombre = "cierre_" + fecha + ".txt";
    ofstream f(nombre.c_str());
    if (f) {
        f << o.str();
        cout << "Cierre guardado en " << nombre << "\n";
    }
}

void opcionAvanzarTiempo() {
    int minutos = leerEntero("Minutos a avanzar: ");
    if (minutos < 0 || minutos > 10000000) {
        cout << "ERROR: valor invalido.\n";
        return;
    }
    desfaseMinutos += minutos;
    guardarDatos();
    cout << "Reloj avanzado " << minutos << " minutos.\n";
}


// Pruebas automaticas (se ejecutan con: ./parqueo --test

int pruebasFalladas = 0;
int pruebasHechas = 0;

void probar(string nombre, bool condicion) {
    pruebasHechas++;
    if (condicion) {
        cout << "  [OK]    " << nombre << "\n";
    } else {
        cout << "  [FALLO] " << nombre << "\n";
        pruebasFalladas++;
    }
}

void modoPrueba() {
    cout << "== Pruebas automaticas ==\n";
    string m;
    Pago ninguno = nuevoPago();

    cout << "\n[1] Calculo de cobro\n";
    probar("carro 10 min = Q0", calcularCobro(CARRO, 10, false) == 0);
    probar("carro 11 min = Q5", calcularCobro(CARRO, 11, false) == 5);
    probar("carro 61 min = Q10", calcularCobro(CARRO, 61, false) == 10);
    probar("carro 10 horas = Q40 (tope)", calcularCobro(CARRO, 600, false) == 40);
    probar("carro 25 horas = Q45", calcularCobro(CARRO, 1500, false) == 45);
    probar("moto 26h 35m = Q29", calcularCobro(MOTO, 1595, false) == 29);
    probar("boleto perdido, carro 25 horas = Q80", calcularCobro(CARRO, 1500, true) == 80);

    cout << "\n[2] Ingresos y salidas\n";
    iniciarEspacios();
    desfaseMinutos = 0;
    probar("ingreso de ABC123", registrarIngreso("ABC123", CARRO, m));
    probar("ingreso repetido falla", !registrarIngreso("ABC123", CARRO, m));
    probar("salida sin ingreso falla", !registrarSalida("NOEXISTE", false, ninguno, ahora(), m));
    probar("salida sin pago falla", !registrarSalida("ABC123", false, ninguno, ahora() + 95, m));
    probar("sigue dentro despues de fallar", buscarPlaca("ABC123") != -1);

    Pago efectivo = nuevoPago();
    efectivo.metodo = EFECTIVO;
    efectivo.recibido = 5;
    probar("efectivo insuficiente falla", !registrarSalida("ABC123", false, efectivo, ahora() + 95, m));
    efectivo.recibido = 20;
    probar("efectivo suficiente funciona", registrarSalida("ABC123", false, efectivo, ahora() + 95, m));
    probar("el recibo muestra el cambio", m.find("Cambio:        Q10.00") != string::npos);
    probar("el espacio quedo libre", buscarPlaca("ABC123") == -1);
    probar("no se puede salir dos veces", !registrarSalida("ABC123", false, efectivo, ahora() + 95, m));

    cout << "\n[3] Pago con tarjeta\n";
    registrarIngreso("TARJ01", CARRO, m);
    Pago tarjeta = nuevoPago();
    tarjeta.metodo = TARJETA;
    tarjeta.last4 = "12";
    tarjeta.aut = "AUT12345";
    probar("tarjeta con 2 digitos falla", !registrarSalida("TARJ01", false, tarjeta, ahora() + 95, m));
    tarjeta.last4 = "1234";
    tarjeta.aut = "x";
    probar("autorizacion invalida falla", !registrarSalida("TARJ01", false, tarjeta, ahora() + 95, m));
    tarjeta.aut = "AUT12345";
    probar("tarjeta correcta funciona", registrarSalida("TARJ01", false, tarjeta, ahora() + 95, m));
    probar("el recibo muestra los 4 digitos", m.find("**** 1234") != string::npos);

    registrarIngreso("GRACIA", MOTO, m);
    probar("dentro de la gracia no pide pago", registrarSalida("GRACIA", false, ninguno, ahora() + 5, m));
    probar("pagos del historial coherentes", verificarPagos());

    cout << "\n[4] Capacidad\n";
    iniciarEspacios();
    bool todasMotos = true;
    for (int i = 0; i < 5; i++) {
        if (!registrarIngreso("MOTO" + to_string(i), MOTO, m)) todasMotos = false;
    }
    probar("caben 5 motos", todasMotos);
    probar("la moto 6 no cabe", !registrarIngreso("MOTO5", MOTO, m));
    bool todosCarros = true;
    for (int i = 0; i < 10; i++) {
        if (!registrarIngreso("CARR" + to_string(i), CARRO, m)) todosCarros = false;
    }
    probar("caben 10 carros", todosCarros);
    probar("el carro 11 no cabe", !registrarIngreso("CARR10", CARRO, m));
    bool todosCamiones = true;
    for (int i = 0; i < 3; i++) {
        if (!registrarIngreso("CAM" + to_string(i), CAMION, m)) todosCamiones = false;
    }
    probar("caben 3 camiones", todosCamiones);
    probar("el camion 4 no cabe", !registrarIngreso("CAM3", CAMION, m));
    probar("el estado es consistente", verificarEstado());

    cout << "\n[5] Simulacion de 5000 operaciones al azar\n";
    iniciarEspacios();
    srand(12345);
    int errores = 0;
    for (int n = 0; n < 5000; n++) {
        string placa = "SIM" + to_string(rand() % 40);
        bool estabaDentro = (buscarPlaca(placa) != -1);
        if (rand() % 100 < 50) {
            bool ok = registrarIngreso(placa, rand() % 3 + 1, m);
            if (estabaDentro && ok) errores++;   // no debe dejar entrar dos veces
        } else {
            Pago p = nuevoPago();
            p.metodo = EFECTIVO;
            p.recibido = 1000;
            bool perdido = (rand() % 10 == 0);
            bool ok = registrarSalida(placa, perdido, p, ahora() + rand() % 3000, m);
            if (ok != estabaDentro) errores++;   // solo sale quien estaba adentro
        }
        if (!verificarEstado()) errores++;
    }
    probar("la simulacion no encontro errores", errores == 0);
    probar("pagos del historial coherentes", verificarPagos());

    cout << "\nPruebas: " << pruebasHechas << " | Fallos: " << pruebasFalladas << "\n";
    if (pruebasFalladas == 0) {
        cout << "TODAS LAS PRUEBAS PASARON\n";
    } else {
        cout << "HAY PRUEBAS FALLIDAS\n";
    }
}

// Programa principal

int main(int argc, char *argv[]) {
    if (argc > 1 && string(argv[1]) == "--test") {
        modoPrueba();
        return pruebasFalladas == 0 ? 0 : 1;
    }

    iniciarEspacios();
    int resultado = cargarDatos();
    if (resultado == 1) {
        int dentro = 0;
        for (int i = 0; i < MAX_ESPACIOS; i++) {
            if (espacios[i].ocupado) dentro++;
        }
        cout << "Datos anteriores cargados: " << dentro << " vehiculos dentro, "
             << totalHistorial << " salidas en el historial.\n";
    } else if (resultado == -1) {
        // el archivo estaba danado: se guarda una copia y se empieza de cero
        string copia = string(ARCHIVO) + ".bak";
        remove(copia.c_str());
        rename(ARCHIVO, copia.c_str());
        iniciarEspacios();
        desfaseMinutos = 0;
        cout << "AVISO: el archivo de datos estaba danado. Se empieza vacio y la copia\n"
             << "quedo en " << copia << ".\n";
    }

    int opcion = -1;
    do {
        mostrarMenu();
        opcion = leerEntero("Opcion: ");
        switch (opcion) {
            case 1: opcionIngreso(); break;
            case 2: opcionSalida(); break;
            case 3: opcionDisponibilidad(); break;
            case 4: opcionMapa(); break;
            case 5: opcionConsultar(); break;
            case 6: opcionTarifas(); break;
            case 7: opcionCierre(); break;
            case 8: opcionAvanzarTiempo(); break;
            case 0: break;
            default: cout << "Opcion invalida.\n";
        }
        // despues de cada operacion se comprueba que todo siga bien
        if (!verificarEstado()) {
            cout << "FALLO DE INTEGRIDAD: los datos quedaron inconsistentes. Se detiene el programa.\n";
            return 1;
        }
    } while (opcion != 0);

    cout << "Hasta luego.\n";
    return 0;
}
