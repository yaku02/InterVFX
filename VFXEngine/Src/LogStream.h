#pragma once

class LogStream
{
public:
    static LogStream& Get()
    {
        static LogStream instance;
        return instance;
    }

    void AddLog(const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logs.push_back(msg);

        // ログが増えすぎたら古いものを削除（最大1000行）
        if (m_logs.size() > 1000)
        {
            m_logs.erase(m_logs.begin());
        }
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logs.clear();
    }

    void Draw(const char* title = "Log Console")
    {
        ImGui::Begin(title);

        if (ImGui::Button("Clear")) { Clear(); }
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& log : m_logs)
        {
            ImGui::TextUnformatted(log.c_str());
        }

        // 自動スクロール（最下部へ）
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
        ImGui::End();
    }

private:
    std::vector<std::string> m_logs;
    std::mutex m_mutex;
};