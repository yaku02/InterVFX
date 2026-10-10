#pragma once

class PreviewCamera {
public:
    // 初期化（FOV、アスペクト比、Near/Far面）
    static void Init(float fovAngleY, float aspectRatio, float nearZ, float farZ);

    // アスペクト比の更新（画面サイズ変更時など）
    static void SetAspectRatio(float aspectRatio);

    // 毎フレームの更新（固定カメラのため移動・回転入力は行わないが、必要に応じた更新処理用）
    static void Update(float deltaTime);

    // --- 各種行列・パラメータの取得 ---
    static DirectX::XMMATRIX GetViewMatrix();
    static DirectX::XMMATRIX GetProjMatrix();
    static DirectX::XMMATRIX GetViewProjMatrix();

    // ConstantBuffer (GlobalCB) への送信用
    static DirectX::XMFLOAT4X4 GetViewProjFloat4x4();

    // --- パラメータの設定・取得 ---
    static DirectX::XMFLOAT3 GetPosition() { return s_position; }
    static void SetPosition(const DirectX::XMFLOAT3& pos);

    // 位置と注視点を指定してカメラの姿勢を決定する
    static void SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f));

private:
    static void UpdateViewMatrix();
    static void UpdateProjectionMatrix();

    // static メンバ変数（C++17 inline static）
    inline static DirectX::XMFLOAT4X4 s_viewMatrix{};
    inline static DirectX::XMFLOAT4X4 s_projMatrix{};

    // 固定カメラのデフォルト位置（必要に応じて変更してください）
    inline static DirectX::XMFLOAT3 s_position{ 0.0f, 2.0f, -10.0f };
    // 注視点計算用、または固定の向き
    inline static float s_yaw = 0.0f;
    inline static float s_pitch = 0.0f;

    inline static float s_fovAngleY = DirectX::XMConvertToRadians(45.0f);
    inline static float s_aspectRatio = 16.0f / 9.0f;
    inline static float s_nearZ = 0.1f;
    inline static float s_farZ = 1000.0f;
};