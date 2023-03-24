#pragma once

#include <optional>
#include <unordered_map>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * В качестве источника для идей предлагаем взглянуть на нашу версию обработчика запросов.
 * Вы можете реализовать обработку запросов способом, который удобнее вам.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)
namespace transport {

    using BusPtr = Bus*;
    struct BusStat {
        size_t route_length;
        double curvature;
        size_t stop_count;
        size_t unique_stop_count;
    };

    using router::Router;

    class RequestHandler {
    public:
        // MapRender понадобится в следующей части итогового проекта
        RequestHandler(const TransportCatalogue& db, const renderer::MapRender& renderer, const router::Router& router);

        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<BusStat> GetBusStat(const std::string_view bus_name) const;

        // Возвращает маршруты, проходящие через
        const std::optional <std::set<std::string_view>> GetSortedBusesByStop(const std::string_view stop_name) const;

        std::optional<Router::RouteInfo> BuildRoute(const std::string_view from, const std::string_view to) const;

        std::string RenderMap() const;
        const TransportCatalogue& GetTransportCatalogue() const;
    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        const TransportCatalogue& db_;
        const renderer::MapRender& renderer_;
        const router::Router& router_;

        //mutable std::unordered_map<std::string_view, std::optional<BusStat>> bus_stat_cash_;
        //mutable std::unordered_map<std::string_view, std::optional <std::set<std::string_view>>> stop_stat_cash_;

        const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;
    };
}//namespace transport
