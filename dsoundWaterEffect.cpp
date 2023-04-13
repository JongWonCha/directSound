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
    // DirectSound 객체 생성
    DirectSoundCreate8(NULL, &g_lpDS, NULL);
    g_lpDS->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1;
    waveFormat.nSamplesPerSec = 44100; //샘플링 주파수 44100hz
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = waveFormat.nChannels * (waveFormat.wBitsPerSample / 8); // 블록 정렬 값 계산
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign; // 평균 바이트 전송률 계산
    waveFormat.cbSize = 0; //추가 정보 없음. 추후 학습
    BYTE* waveData = nullptr;
    DWORD waveDataSize = 491516;//단위 바이트
    LoadWaveFile("Alarm01.wav", waveFormat, waveData, waveDataSize);

    // 사운드 버퍼 생성
    DSBUFFERDESC dsbd;
    memset(&dsbd, 0, sizeof(dsbd));
    dsbd.dwSize = sizeof(dsbd);
    dsbd.dwFlags = DSBCAPS_CTRLFX | DSBCAPS_GLOBALFOCUS;
    dsbd.dwBufferBytes = waveDataSize;
    dsbd.lpwfxFormat = &waveFormat;

    g_lpDS->CreateSoundBuffer(&dsbd, &g_lpDSBuffer, NULL);
    g_lpDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&g_lpDSBuffer8);

    // 사운드 버퍼에 웨이브 데이터 write
    VOID* audioPtr1;
    DWORD audioBytes1;
    VOID* audioPtr2;
    DWORD audioBytes2;
    g_lpDSBuffer8->Lock(0, waveDataSize, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, 0);
    memcpy(audioPtr1, waveData, audioBytes1);
    memcpy(audioPtr2, waveData + audioBytes1, audioBytes2);
    g_lpDSBuffer8->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);

    // 사운드 버퍼에 이퀄라이저 효과 추가
    DSEFFECTDESC dsfx;
    memset(&dsfx, 0, sizeof(dsfx));
    dsfx.dwSize = sizeof(dsfx);
    dsfx.guidDSFXClass = GUID_DSFX_STANDARD_PARAMEQ;
    dsfx.dwFlags = DSFX_LOCSOFTWARE;
    dsfx.dwReserved1 = 0;
    dsfx.dwReserved2 = 0;
    g_lpDSBuffer8->SetFX(1, &dsfx, NULL);

    // 이퀄라이저 인터페이스 가져오기
    g_lpDSBuffer8->GetObjectInPath(GUID_DSFX_STANDARD_PARAMEQ, 0, IID_IDirectSoundFXParamEq, (LPVOID*)&g_lpDSFXParamEq);

    // 이퀄라이저 효과 설정 중심 주파수를 정하고 bandwidth만큼의 주파수의 Gain를 낮춰 먹먹한 사운드 구현
    DSFXParamEq eqParams;
    eqParams.fCenter = 10000.0f; //중심이 되는 Hz
    eqParams.fBandwidth = 12.0f; //옥타브 단위
    eqParams.fGain = -12.0f; // 데시벨 단위
    g_lpDSFXParamEq->SetAllParameters(&eqParams);

    // 물속 효과 재생
    PlayWaterEffect();

    // 사용자 입력 대기, 입력시 종료와 릴리즈
    std::cout << "Press any key to stop playing..." << std::endl;
    _getch();

    // 정리
    CleanUp();

    return 0;
}

void LoadWaveFile(const char* filename, WAVEFORMATEX& waveFormat, BYTE*& data, DWORD& dataSize) {  // 웨이브 파일 로드 로직 구현
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "파일을 열수 없음! " << filename << std::endl;
        return;
    }

    // RIFF 헤더 확인
    char chunkID[4];
    file.read(chunkID, 4);
    if (strncmp(chunkID, "RIFF", 4) != 0) {
        std::cerr << "Invalid wave file: " << filename << std::endl;
        return;
    }

    file.seekg(4, std::ios::cur);  // 파일 크기 무시

    // WAVE 헤더 확인
    char format[4];
    file.read(format, 4);
    if (strncmp(format, "WAVE", 4) != 0) {
        std::cerr << "Invalid wave file: " << filename << std::endl;
        return;
    }

    // fmt 청크 찾기
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

    // data 청크 찾기
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
    g_lpDSBuffer8->Play(0, 0, DSBPLAY_LOOPING); // 사운드 버퍼 재생, DSBPLAY_LOOPING 루프 재생
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