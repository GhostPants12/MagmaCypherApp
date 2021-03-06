#include <iostream>
#include <iomanip>
#include <napi.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

static unsigned char Pi[8][16] =
{
  {1,7,14,13,0,5,8,3,4,15,10,6,9,12,11,2},
  {8,14,2,5,6,9,1,12,15,4,11,0,13,10,3,7},
  {5,13,15,6,9,2,12,10,11,7,8,1,4,3,14,0},
  {7,15,5,10,8,1,6,13,0,9,3,14,11,4,2,12},
  {12,8,2,1,13,4,15,6,7,0,10,5,3,14,9,11},
  {11,3,5,8,2,15,10,13,14,1,7,4,12,9,6,0},
  {6,8,2,3,9,10,5,12,1,14,4,7,11,13,0,15},
  {12,4,6,2,10,5,11,9,14,8,13,7,0,3,15,1}
};

const short CYPHER_BLOCK_SIZE = 8;
const short BLOCK_SIZE = 4;
typedef uint8_t vect[BLOCK_SIZE];
typedef uint8_t cyphblck[CYPHER_BLOCK_SIZE];

const uint8_t key[32] = {
    12, 20, 24, 67,
    43, 25, 28, 37,
    121, 203, 204, 35,
    21, 144, 224, 167,
    102, 120, 124, 70,
    45, 87, 142, 98,
    36, 21, 28, 95,
    100, 200, 150, 50
};

vect iter_key[32];

void GOST_Magma_Expand_Key(const uint8_t* key, Napi::Env env, Napi::Object keys)
{
    // Формируем восемь 32-битных подключей в порядке следования с первого по восьмой
    memcpy(iter_key[0], key, 4);
    memcpy(iter_key[1], key + 4, 4);
    memcpy(iter_key[2], key + 8, 4);
    memcpy(iter_key[3], key + 12, 4);
    memcpy(iter_key[4], key + 16, 4);
    memcpy(iter_key[5], key + 20, 4);
    memcpy(iter_key[6], key + 24, 4);
    memcpy(iter_key[7], key + 28, 4);
    memcpy(iter_key[8], key, 4);
    memcpy(iter_key[9], key + 4, 4);
    memcpy(iter_key[10], key + 8, 4);
    memcpy(iter_key[11], key + 12, 4);
    memcpy(iter_key[12], key + 16, 4);
    memcpy(iter_key[13], key + 20, 4);
    memcpy(iter_key[14], key + 24, 4);
    memcpy(iter_key[15], key + 28, 4);
    memcpy(iter_key[16], key, 4);
    memcpy(iter_key[17], key + 4, 4);
    memcpy(iter_key[18], key + 8, 4);
    memcpy(iter_key[19], key + 12, 4);
    memcpy(iter_key[20], key + 16, 4);
    memcpy(iter_key[21], key + 20, 4);
    memcpy(iter_key[22], key + 24, 4);
    memcpy(iter_key[23], key + 28, 4);
    memcpy(iter_key[24], key + 28, 4);
    memcpy(iter_key[25], key + 24, 4);
    memcpy(iter_key[26], key + 20, 4);
    memcpy(iter_key[27], key + 16, 4);
    memcpy(iter_key[28], key + 12, 4);
    memcpy(iter_key[29], key + 8, 4);
    memcpy(iter_key[30], key + 4, 4);
    memcpy(iter_key[31], key, 4);
    Napi::Array keyArr = Napi::Array::New(env);
    char buff[60];
    for (int i = 0; i < 32; i++) {
        snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X", iter_key[i][0], iter_key[i][1], iter_key[i][2], iter_key[i][3]);
        keyArr.Set(i, buff);
    }

    keys.Set("keys", keyArr);
}

static void GOST_Magma_Add(const uint8_t* a, const uint8_t* b, uint8_t* c)
{
    int i;
    for (i = 0; i < BLOCK_SIZE; i++)
        c[i] = a[i] ^ b[i];
}

static void GOST_Magma_Add_32(const uint8_t* a, const uint8_t* b, uint8_t* c)
{
    int i;
    unsigned int internal = 0;
    for (i = 3; i >= 0; i--)
    {
        internal = a[i] + b[i] + (internal >> 8);
        c[i] = internal & 0xff;
    }
}

static void GOST_Magma_T(const uint8_t* in_data, uint8_t* out_data)
{
    uint8_t first_part_byte, sec_part_byte;
    int i;
    for (i = 0; i < 4; i++)
    {
        // Извлекаем первую 4-битную часть байта
        first_part_byte = (in_data[i] & 0xf0) >> 4;
        // Извлекаем вторую 4-битную часть байта
        sec_part_byte = (in_data[i] & 0x0f);
        // Выполняем замену в соответствии с таблицей подстановок
        first_part_byte = Pi[i * 2][first_part_byte];
        sec_part_byte = Pi[i * 2 + 1][sec_part_byte];
        // «Склеиваем» обе 4-битные части обратно в байт
        out_data[i] = (first_part_byte << 4) | sec_part_byte;
    }
}

static void GOST_Magma_g(const uint8_t* k, const uint8_t* a, uint8_t* out_data)
{
    uint8_t internal[4];
    uint32_t out_data_32;
    // Складываем по модулю 32 правую половину блока с итерационным ключом
    GOST_Magma_Add_32(a, k, internal);
    // Производим нелинейное биективное преобразование результата
    GOST_Magma_T(internal, internal);
    // Преобразовываем четырехбайтный вектор в одно 32-битное число
    out_data_32 = internal[0];
    out_data_32 = (out_data_32 << 8) + internal[1];
    out_data_32 = (out_data_32 << 8) + internal[2];
    out_data_32 = (out_data_32 << 8) + internal[3];
    // Циклически сдвигаем все влево на 11 разрядов
    out_data_32 = (out_data_32 << 11) | (out_data_32 >> 21);
    // Преобразовываем 32-битный результат сдвига обратно в 4-байтовый вектор
    out_data[3] = out_data_32;
    out_data[2] = out_data_32 >> 8;
    out_data[1] = out_data_32 >> 16;
    out_data[0] = out_data_32 >> 24;
}

static void GOST_Magma_G(const uint8_t* k, const uint8_t* a, uint8_t* out_data)
{
    uint8_t a_0[4]; // Правая половина блока
    uint8_t a_1[4]; // Левая половина блока
    uint8_t G[4];

    int i;
    // Делим 64-битный исходный блок на две части
    for (i = 0; i < 4; i++)
    {
        a_0[i] = a[4 + i];
        a_1[i] = a[i];
    }

    // Производим преобразование g
    GOST_Magma_g(k, a_0, G);
    // Ксорим результат преобразования g с левой половиной блока
    GOST_Magma_Add(a_1, G, G);

    for (i = 0; i < 4; i++)
    {
        // Пишем в левую половину значение из правой
        a_1[i] = a_0[i];
        // Пишем результат GOST_Magma_Add в правую половину блока
        a_0[i] = G[i];
    }

    // Сводим правую и левую части блока в одно целое
    for (i = 0; i < 4; i++)
    {
        out_data[i] = a_1[i];
        out_data[4 + i] = a_0[i];
    }
}

static void GOST_Magma_G_Fin(const uint8_t* k, const uint8_t* a, uint8_t* out_data)
{
    uint8_t a_0[4]; // Правая половина блока
    uint8_t a_1[4]; // Левая половина блока
    uint8_t G[4];

    int i;
    // Делим 64-битный исходный блок на две части
    for (i = 0; i < 4; i++)
    {
        a_0[i] = a[4 + i];
        a_1[i] = a[i];
    }

    // Производим преобразование g
    GOST_Magma_g(k, a_0, G);
    // Ксорим результат преобразования g с левой половиной блока
    GOST_Magma_Add(a_1, G, G);
    // Пишем результат GOST_Magma_Add в левую половину блока
    for (i = 0; i < 4; i++)
        a_1[i] = G[i];

    // Сводим правую и левую части блока в одно целое
    for (i = 0; i < 4; i++)
    {
        out_data[i] = a_1[i];
        out_data[4 + i] = a_0[i];
    }
}

void GOST_Magma_Encrypt(const uint8_t* blk, uint8_t* out_blk, Napi::Env env, Napi::Array crypt, int index)
{
    int i = 0;
    char buff[60];
    Napi::Object obj = Napi::Object::New(env);
    Napi::Array arrBef = Napi::Array::New(env);
    Napi::Array arrAft = Napi::Array::New(env);
    // Первое преобразование G
    obj.Set("type", "Encryption");
    snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        blk[0], blk[1], blk[2], blk[3], blk[4], blk[5], blk[6], blk[7]);
    arrBef.Set(i, buff);
    GOST_Magma_G(iter_key[0], blk, out_blk);
    snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
    arrAft.Set(i, buff);
    // Последующие (со второго по тридцать первое) преобразования G
    for (i = 1; i < 31; i++){
        snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
        arrBef.Set(i, buff);
        GOST_Magma_G(iter_key[i], out_blk, out_blk);
        snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
        arrAft.Set(i, buff);
    }
    // Последнее (тридцать второе) преобразование G
    snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
    arrBef.Set(i, buff);
    GOST_Magma_G_Fin(iter_key[31], out_blk, out_blk);
    snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
    arrAft.Set(i, buff);
    obj.Set("bef", arrBef);
    obj.Set("aft", arrAft);
    crypt.Set(index, obj);
}

void GOST_Magma_Decrypt(const uint8_t* blk, uint8_t* out_blk, Napi::Env env, Napi::Array crypt, int index)
{
    int i = 31;
    char buff[60];
    Napi::Object obj = Napi::Object::New(env);
    Napi::Array arrBef = Napi::Array::New(env);
    Napi::Array arrAft = Napi::Array::New(env);
    // Первое преобразование G с использованием
    // тридцать второго итерационного ключа
    obj.Set("type", "Decryption");
    snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        blk[0], blk[1], blk[2], blk[3], blk[4], blk[5], blk[6], blk[7]);
    arrBef.Set(i, buff);
    GOST_Magma_G(iter_key[31], blk, out_blk);
    snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
    arrAft.Set(i, buff);
    // Последующие (со второго по тридцать первое) преобразования G
    // (итерационные ключи идут в обратном порядке)
    for (i = 30; i > 0; i--)
    {
        snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
        arrBef.Set(i, buff);
        GOST_Magma_G(iter_key[i], out_blk, out_blk);
        snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
        arrAft.Set(i, buff);
    }
    // Последнее (тридцать второе) преобразование G
    // с использованием первого итерационного ключа
    arrBef.Set(i, buff);
    GOST_Magma_G_Fin(iter_key[0], out_blk, out_blk);
    snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
        out_blk[0], out_blk[1], out_blk[2], out_blk[3], out_blk[4], out_blk[5], out_blk[6], out_blk[7]);
    arrAft.Set(i, buff);
    obj.Set("bef", arrBef);
    obj.Set("aft", arrAft);
    crypt.Set(index, obj);
}

Napi::Object Encrypt(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return Napi::TypedArrayOf<uint8_t>::New(env, {1});
    }

    if(!info[0].IsTypedArray()){
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return Napi::TypedArrayOf<uint8_t>::New(env, {1});
    }

    Napi::Object obj = Napi::Object::New(env);
    Napi::Object keyGen = Napi::Object::New(env);

    if(info.Length() > 1){
        if(!info[1].IsTypedArray()){
            Napi::TypeError::New(env, "Wrong key").ThrowAsJavaScriptException();
            return Napi::TypedArrayOf<uint8_t>::New(env, {1});
        }

        Napi::TypedArrayOf<uint8_t> paramKey = info[1].As<Napi::TypedArrayOf<uint8_t>>();
        int bufferLength = paramKey.ElementLength();
        if(bufferLength == 32){
            vector<uint8_t> customKey;
            customKey.resize(bufferLength);
            memcpy(&customKey[0], &paramKey[0], bufferLength);
            char buff[150];
            keyGen.Set("keyType", "Using custom key");
            snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            customKey[0], customKey[1], customKey[2], customKey[3], customKey[4], customKey[5], customKey[6], customKey[7],
            customKey[8], customKey[9], customKey[10], customKey[11], customKey[12], customKey[13], customKey[14], customKey[15],
            customKey[16], customKey[17], customKey[18], customKey[19], customKey[20], customKey[21], customKey[22], customKey[23],
            customKey[24], customKey[25], customKey[26], customKey[27], customKey[28], customKey[29], customKey[30], customKey[31]);
            keyGen.Set("keyValue", buff);
            GOST_Magma_Expand_Key(&customKey[0], env, keyGen);
        }
        else {
            keyGen.Set("keyType", "Using default key");
            GOST_Magma_Expand_Key(key, env, keyGen);
        }
    }
    else{
        keyGen.Set("keyType", "Using custom key");
        GOST_Magma_Expand_Key(key, env, keyGen);
    }
    obj.Set("keyGen", keyGen);


    Napi::TypedArrayOf<uint8_t> arr = info[0].As<Napi::TypedArrayOf<uint8_t>>();
    int bufferLength = arr.ElementLength();
    vector<uint8_t> msg;
    for(int i = 0; i < bufferLength; i++){
        msg.push_back(arr[i]);
    }

    vector<uint8_t> cyphMsg;
    msg.push_back(1);
    char buff[60];

    if(msg.size() % 8 != 0){
        int size = msg.size();
        for(int i = 0; i< 8-(size % 8); i++)
        {
            msg.push_back(0);
        }
    }

    cyphMsg.resize(msg.size());

    cyphblck inblck;
    cyphblck outblck;
    int l = 0;
    Napi::Array arrObj = Napi::Array::New(env);

    while((l+1) < msg.size()){
        memcpy(inblck, &msg[l], 8);
        GOST_Magma_Encrypt(inblck, outblck, env, arrObj, l/8);
        memcpy(&cyphMsg[l], outblck, 8);
        l+=8;
    }

    obj.Set("blocks", arrObj);
    Napi::TypedArrayOf<uint8_t> result = Napi::TypedArrayOf<uint8_t>::New(env, cyphMsg.size());
    memcpy(result.Data(), &cyphMsg[0], cyphMsg.size());
    obj.Set("result", result);
    return obj;
}

Napi::Object Decrypt(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return Napi::TypedArrayOf<uint8_t>::New(env, {1});
    }

    if(!info[0].IsTypedArray()){
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return Napi::TypedArrayOf<uint8_t>::New(env, {1});
    }

    Napi::Object obj = Napi::Object::New(env);
    Napi::Object keyGen = Napi::Object::New(env);


    if(info.Length() > 1){
        if(!info[1].IsTypedArray()){
            Napi::TypeError::New(env, "Wrong key").ThrowAsJavaScriptException();
            return Napi::TypedArrayOf<uint8_t>::New(env, {1});
        }

        Napi::TypedArrayOf<uint8_t> paramKey = info[1].As<Napi::TypedArrayOf<uint8_t>>();
        int bufferLength = paramKey.ElementLength();
        if(bufferLength == 32){
            vector<uint8_t> customKey;
            customKey.resize(bufferLength);
            memcpy(&customKey[0], &paramKey[0], bufferLength);
            char buff[150];
            keyGen.Set("keyType", "Using custom key");
            snprintf(buff, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            customKey[0], customKey[1], customKey[2], customKey[3], customKey[4], customKey[5], customKey[6], customKey[7],
            customKey[8], customKey[9], customKey[10], customKey[11], customKey[12], customKey[13], customKey[14], customKey[15],
            customKey[16], customKey[17], customKey[18], customKey[19], customKey[20], customKey[21], customKey[22], customKey[23],
            customKey[24], customKey[25], customKey[26], customKey[27], customKey[28], customKey[29], customKey[30], customKey[31]);
            keyGen.Set("keyValue", buff);
            GOST_Magma_Expand_Key(&customKey[0], env, keyGen);
        }
        else {
            keyGen.Set("keyType", "Using default key");
            GOST_Magma_Expand_Key(key, env, keyGen);
        }
    }
    else{
        keyGen.Set("keyType", "Using default key");
        GOST_Magma_Expand_Key(key, env, keyGen);
    }
    obj.Set("keyGen", keyGen);


    Napi::TypedArrayOf<uint8_t> arr = info[0].As<Napi::TypedArrayOf<uint8_t>>();
    int bufferLength = arr.ElementLength();
    vector<uint8_t> msg;
    msg.resize(bufferLength);
    memcpy(&msg[0], &arr[0], bufferLength);

    vector<uint8_t> cyphMsg;
    char buff[60];

    cyphMsg.resize(msg.size());

    cyphblck inblck;
    cyphblck outblck;
    int l = 0;
    Napi::Array objArr = Napi::Array::New(env);

    while((l+1) < msg.size()){
        memcpy(inblck, &msg[l], 8);
        GOST_Magma_Decrypt(inblck, outblck, env, objArr, l/8);
        memcpy(&cyphMsg[l], outblck, 8);
        l+=8;
    }

    int lastIndex = -1;
    for(int i = 0; i < cyphMsg.size(); i++){
        if(cyphMsg[i] == 1){
            lastIndex = i;
        }
    }

    if(lastIndex != -1){
        cyphMsg.resize(lastIndex);
    }

    obj.Set("blocks", objArr);
    Napi::TypedArrayOf<uint8_t> result = Napi::TypedArrayOf<uint8_t>::New(env, cyphMsg.size());
    memcpy(result.Data(), &cyphMsg[0], cyphMsg.size());
    obj.Set("result", result);
    return obj;
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "encrypt"), Napi::Function::New(env, Encrypt));
    exports.Set(Napi::String::New(env, "decrypt"), Napi::Function::New(env, Decrypt));
    return exports;
};

NODE_API_MODULE(magma_cypher, init);