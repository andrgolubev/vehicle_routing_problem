#include "savings.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <tuple>

namespace vrp {
namespace detail {
std::vector<Solution> savings(const Problem& prob, size_t count) {

    std::cout << "SAVINGS HEURISTIC" << std::endl;

    const auto cust_size = prob.customers.size() - 1;
    const auto points = cust_size + 1;  // + 1 for depot itself

    // vehicle id <-> customets ids
    std::vector<std::pair<size_t, std::vector<size_t>>> routes;
    routes.reserve(cust_size);

    // valid routes
    std::vector<size_t> valid(cust_size, 1);
    valid[0] = 0;

    // visited customers
    std::vector<size_t> dest(points, 0);
    dest[0] = 1;

    // started from customers
    std::vector<size_t> src(points, 0);
    src[0] = 1;

    // backward edges
    std::vector<size_t> back_ed(points, 0);

    // start initialisation
    std::cout << "INIT ROUTES" << std::endl;
    for (unsigned int i = 0; i < points; ++i) {
        routes.push_back({i, {0, i, 0}});
    }

    // cust id <-> route
    std::map<size_t, std::vector<size_t>*> route_map;

    for (unsigned int i = 0; i < points; ++i) {
        route_map[i] = &routes[i].second;
    }

    // saving for i and j vertices
    struct S {
        unsigned int i;
        unsigned int j;
        double save_ij;
    };

    std::vector<S> save(points * points, {0, 0, 0});

    // calculating matrix(vector) of savings
    for (unsigned int i = 0; i < points; ++i) {
        for (unsigned int j = 0; j < points; ++j) {
            save[i * points + j] = {
                i, j, prob.costs[i][0] + prob.costs[0][j] - prob.costs[i][j]};
        }
    }

    // sort and erase nonreq savings
    std::sort(save.begin(), save.end(),
              [](const S& a, const S& b) { return a.save_ij < b.save_ij; });
    save.erase(std::remove_if(save.begin(), save.end(),
                              [](const auto& o) {
                                  return (o.i == o.j || o.i == 0 || o.j == 0);
                              }),
               save.end());

    // savings
    std::cout << std::endl << "SAVINGS I J" << std::endl;
    for (auto a : save) {
        std::cout << "saving " << a.i << " to " << a.j << " with save "
                  << a.save_ij << std::endl;
    }
    std::cout << std::endl;

    // times for customer
    struct Time {
        unsigned int start;
        unsigned int finish;
        unsigned int current;
    };

    // customer id <-> hard tw
    std::map<size_t, Time> times;

    // DELETE ME (cout)
    std::cout << "TIMES (id start finish current)" << std::endl;
    for (auto i = 0; i < points; ++i) {
        times[prob.customers[i].id] = {
            (unsigned int)prob.customers[i].hard_tw.first,
            (unsigned int)prob.customers[i].hard_tw.second,
            /*std::min((unsigned int)prob.customers[i].hard_tw.first,
                     (unsigned int)prob.times[0][prob.customers[i].id])};*/
            (unsigned int)prob.times[0][prob.customers[i].id]};
        std::cout << prob.customers[i].id << ' '
                  << times[prob.customers[i].id].start << ' '
                  << times[prob.customers[i].id].finish << ' '
                  << times[prob.customers[i].id].current << std::endl;
    }

    std::vector<S> fine;
    int kk = 0;
    std::cout << "START" << std::endl;
    while (kk < save.size()) {  // main cycle
        auto best_save = save[save.size() - 1 - kk++];
        // std::cout << "considering save " << best_save.i << " to " <<
        // best_save.j << " with save " << best_save.save_ij << std::endl;

        // cust is not visited yet
        if (!src[best_save.i] && !dest[best_save.j] &&
            (best_save.j != best_save.i) &&
            !(back_ed[best_save.i] == best_save.j)) {

            /*std::cout << "routes:" << std::endl;
            for (auto& r : routes) {
                for (auto& c : r.second) {
                    std::cout << c << ' ';
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;*/

            // handle routes & machines
            // std::cout << "GOOD EDGE TO LOOK AT " << best_save.i << " to " <<
            // best_save.j << " with save " << best_save.save_ij << std::endl;
            // fine.push_back(best_save);

            int time_diff = times[best_save.j].current -
                            times[best_save.i].current -
                            prob.times[best_save.i][best_save.j];
            if (time_diff >= 0) {
                // fine

                std::cout << "EASY INSERT going from i = " << best_save.i
                          << " to j = " << best_save.j << std::endl;
                fine.push_back(best_save);
                dest[best_save.j] = 1;
                src[best_save.i] = 1;
                back_ed[best_save.j] = best_save.i;
                times[best_save.j].current =
                    times[best_save.i].current +
                    prob.times[best_save.i][best_save.j];
                /*routes[best_save.i].second.insert(
                    routes[best_save.i].second.end() - 2, best_save.j);*/

                route_map[best_save.i]->insert(route_map[best_save.i]->end() - 1, best_save.j);
                valid[best_save.j] = 0;
                route_map[best_save.j] = route_map[best_save.i];
            } else {
                int offset = times[best_save.i].current +
                             prob.times[best_save.i][best_save.j] -
                             times[best_save.j].current;
                // check the rest of root with offset
                // std::cout << "offset of going from i = " << best_save.i << "
                // to j = " << best_save.j << " with offset " << offset <<
                // std::endl;
                // offset is ok for j-route
                int flag = 0;
                // std::cout << "j root of "<< best_save.j << std::endl;

                // auto j_route = routes[best_save.j];

                // ckeck offset for that j-route
                for (auto cust : *route_map[best_save.j]) {
                    if (cust != 0 &&
                        (times[cust].current + offset > times[cust].finish)) {
                        flag = 1;
                        break;
                    }
                }

                if (!flag) {
                    std::cout << "TOUGH INSERT going from i = " << best_save.i
                              << " to j = " << best_save.j << std::endl;
                    dest[best_save.j] = 1;
                    src[best_save.i] = 1;
                    back_ed[best_save.j] = best_save.i;
                    fine.push_back(best_save);
                    valid[best_save.j] = 0;
                    for (auto cust : *route_map[best_save.j]) {
                        if (cust != 0) {
                            //std::cout<<cust<<std::endl;
                            times[cust].current += offset;
                            /*routes[best_save.i].second.insert(
                                routes[best_save.i].second.end() - 2, cust);*/
                            route_map[best_save.i]->insert(route_map[best_save.i]->end() - 1, cust);
                            route_map[cust] = route_map[best_save.i];
                        }
                    } 
                }
            }
            /*for (auto a : times) {
                std::cout << a.first << " with current " << a.second.current <<
            std::endl;
            }*/
        }
        // TODO:
        // 1) check capacity
        // 2) somehow manage redundant j route
    }
    std::cout << "Added edges" << std::endl;
    for (auto& a : fine)
        std::cout << a.i << " to " << a.j << " with save " << a.save_ij
                  << std::endl;

    std::cout << std::endl << "final routes" << std::endl;
    // FIXME consider generic approach
    int tmp = 0;
    for (auto a : routes) {
        if (valid[tmp++]) {
            for (auto b : a.second) {
                std::cout << b << ' ';
            }
            std::cout << std::endl;
        }
    }
    // after all
    // put here all the solution stuff

    return {};
}
}  // namespace detail
}  // namespace vrp
