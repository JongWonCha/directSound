#include <Windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <iostream>
#include <conio.h>
#include <cmath>
#include <fstream>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dsound.lib")

LPDIRECTSOUND8 g_lpDS = nullptr;
LPDIRECTSOUNDBUFFER g_lpDSBuffer = nullptr;
LPDIRECTSOUNDBUFFER8 g_lpDSBuffer8 = nullptr;
LPDIRECTSOUNDFXPARAMEQ g_lpDSFXParamEq = nullptr;

void LoadWaveFile(const char* filename, WAVEFORMATEX& waveFormat, BYTE*& data, DWORD& dataSize);
void PlayWaterEffect();
void CleanUp();

int main() {
    // DirectSound ��ü ����
    DirectSoundCreate8(NULL, &g_lpDS, NULL);
    g_lpDS->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1;
    waveFormat.nSamplesPerSec = 44100; //���ø� ���ļ� 44100hz
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = waveFormat.nChannels * (waveFormat.wBitsPerSample / 8); // ��� ���� �� ���
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign; // ��� ����Ʈ ���۷� ���
    waveFormat.cbSize = 0; //�߰� ���� ����. ���� �н�
    BYTE* waveData = nullptr;
    DWORD waveDataSize = 491516;//���� ����Ʈ
    LoadWaveFile("Alarm01.wav", waveFormat, waveData, waveDataSize);

    // ���� ���� ����
    DSBUFFERDESC dsbd;
    memset(&dsbd, 0, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_CTRLFX | DSBCAPS_GLOBALFOCUS;
    dsbd.dwBufferBytes = waveDataSize;
    dsbd.lpwfxFormat = &waveFormat;

    g_lpDS->CreateSoundBuffer(&dsbd, &g_lpDSBuffer, NULL);
    g_lpDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&g_lpDSBuffer8);

    // ���� ���ۿ� ���̺� ������ write
    VOID* audioPtr1;
    DWORD audioBytes1;
    VOID* audioPtr2;
    DWORD audioBytes2;
    g_lpDSBuffer8->Lock(0, waveDataSize, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, 0);
    memcpy(audioPtr1, waveData, audioBytes1);
    memcpy(audioPtr2, waveData + audioBytes1, audioBytes2);
    g_lpDSBuffer8->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);

    // ���� ���ۿ� ���������� ȿ�� �߰�
    DSEFFECTDESC dsfx;
    memset(&dsfx, 0, sizeof(dsfx));
    dsfx.dwSize = sizeof(dsfx);
    dsfx.guidDSFXClass = GUID_DSFX_STANDARD_PARAMEQ;
    dsfx.dwFlags = DSFX_LOCSOFTWARE;
    dsfx.dwReserved1 = 0;
    dsfx.dwReserved2 = 0;
    g_lpDSBuffer8->SetFX(1, &dsfx, NULL);

    // ���������� �������̽� ��������
    g_lpDSBuffer8->GetObjectInPath(GUID_DSFX_STANDARD_PARAMEQ, 0, IID_IDirectSoundFXParamEq, (LPVOID*)&g_lpDSFXParamEq);

    // ���������� ȿ�� ���� �߽� ���ļ��� ���ϰ� bandwidth��ŭ�� ���ļ��� Gain�� ���� �Ը��� ���� ����
    DSFXParamEq eqParams;
    eqParams.fCenter = 10000.0f; //�߽��� �Ǵ� Hz
    eqParams.fBandwidth = 12.0f; //��Ÿ�� ����
    eqParams.fGain = -12.0f; // ���ú� ����
    g_lpDSFXParamEq->SetAllParameters(&eqParams);

    // ���� ȿ�� ���
    PlayWaterEffect();

    // ����� �Է� ���, �Է½� ����� ������
    std::cout << "Press any key to stop playing..." << std::endl;
    _getch();

    // ����
    CleanUp();

    return 0;
}

void LoadWaveFile(const char* filename, WAVEFORMATEX& waveFormat, BYTE*& data, DWORD& dataSize) {  // ���̺� ���� �ε� ���� ����
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "������ ���� ����! " << filename << std::endl;
        return;
    }

    // RIFF ��� Ȯ��
    char chunkID[4];
    file.read(chunkID, 4);
    if (strncmp(chunkID, "RIFF", 4) != 0) {
        std::cerr << "Invalid wave file: " << filename << std::endl;
        return;
    }

    file.seekg(4, std::ios::cur);  // ���� ũ�� ����

    // WAVE ��� Ȯ��
    char format[4];
    file.read(format, 4);
    if (strncmp(format, "WAVE", 4) != 0) {
        std::cerr << "Invalid wave file: " << filename << std::endl;
        return;
    }

    // fmt ûũ ã��
    while (true) {
        file.read(chunkID, 4);
        DWORD chunkSize;
        file.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));

        if (strncmp(chunkID, "fmt ", 4) == 0) {
            file.read(reinterpret_cast<char*>(&waveFormat), sizeof(WAVEFORMATEX));
            break;
        }
        else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    // data ûũ ã��
    while (true) {
        file.read(chunkID, 4);
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(DWORD));

        if (strncmp(chunkID, "data", 4) == 0) {
            data = new BYTE[dataSize];
            file.read(reinterpret_cast<char*>(data), dataSize);
            break;
        }
        else {
            file.seekg(dataSize, std::ios::cur);
        }
    }

    file.close();
}

void PlayWaterEffect() {
    g_lpDSBuffer8->Play(0, 0, DSBPLAY_LOOPING); // ���� ���� ���, DSBPLAY_LOOPING ���� ���
}

void CleanUp() {
    if (g_lpDSFXParamEq) {
        g_lpDSFXParamEq->Release();
        g_lpDSFXParamEq = nullptr;
    }

    if (g_lpDSBuffer8) {
        g_lpDSBuffer8->Release();
        g_lpDSBuffer8 = nullptr;
    }

    if (g_lpDSBuffer) {
        g_lpDSBuffer->Release();
        g_lpDSBuffer = nullptr;
    }

    if (g_lpDS) {
        g_lpDS->Release();
        g_lpDS = nullptr;
    }
}