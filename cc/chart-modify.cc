#include <random>

#include "acmacs-base/omp.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/chart-modify.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

ChartModify::ChartModify(size_t number_of_antigens, size_t number_of_sera)
    : info_{std::make_shared<InfoModify>()},
      antigens_{std::make_shared<AntigensModify>(number_of_antigens)},
      sera_{std::make_shared<SeraModify>(number_of_sera)},
      titers_{std::make_shared<TitersModify>(number_of_antigens, number_of_sera)},
      projections_{std::make_shared<ProjectionsModify>(*this)},
      plot_spec_{std::make_shared<PlotSpecModify>(number_of_antigens, number_of_sera)}
{
} // ChartModify::ChartModify

// ----------------------------------------------------------------------

InfoP ChartModify::info() const
{
    if (info_)
        return info_;
    else
        return main_->info();

} // ChartModify::info

// ----------------------------------------------------------------------

InfoModifyP ChartModify::info_modify()
{
    if (!info_)
        info_ = std::make_shared<InfoModify>(main_->info());
    return info_;

} // ChartModify::info_modify

// ----------------------------------------------------------------------

AntigensP ChartModify::antigens() const
{
    if (antigens_)
        return antigens_;
    else
        return main_->antigens();

} // ChartModify::antigens

// ----------------------------------------------------------------------

AntigensModifyP ChartModify::antigens_modify()
{
    if (!antigens_)
        antigens_ = std::make_shared<AntigensModify>(main_->antigens());
    return antigens_;

} // ChartModify::antigens_modify

// ----------------------------------------------------------------------

SeraP ChartModify::sera() const
{
    if (sera_)
        return sera_;
    else
        return main_->sera();

} // ChartModify::sera

// ----------------------------------------------------------------------

SeraModifyP ChartModify::sera_modify()
{
    if (!sera_)
        sera_ = std::make_shared<SeraModify>(main_->sera());
    return sera_;

} // ChartModify::sera_modify

// ----------------------------------------------------------------------

TitersP ChartModify::titers() const
{
    if (titers_)
        return titers_;
    else
        return main_->titers();

} // ChartModify::titers

// ----------------------------------------------------------------------

TitersModifyP ChartModify::titers_modify()
{
    if (!titers_)
        titers_ = std::make_shared<TitersModify>(main_->titers());
    return titers_;

} // ChartModify::titers_modify

// ----------------------------------------------------------------------

ProjectionsP ChartModify::projections() const
{
    if (projections_)
        return projections_;
    else
        return main_->projections();

} // ChartModify::projections

// ----------------------------------------------------------------------

PlotSpecP ChartModify::plot_spec() const
{
    if (plot_spec_)
        return plot_spec_;
    else
        return main_->plot_spec();

} // ChartModify::plot_spec

// ----------------------------------------------------------------------

ColumnBasesP ChartModify::forced_column_bases() const
{
    if (forced_column_bases_)
        return forced_column_bases_;
    else if (main_)
        return main_->forced_column_bases();
    else
        return {};

} // ChartModify::forced_column_bases

// ----------------------------------------------------------------------

ColumnBasesModifyP ChartModify::forced_column_bases_modify()
{
    if (!forced_column_bases_ && main_) {
        if (auto fcb = main_->forced_column_bases(); fcb)
            forced_column_bases_ = std::make_shared<ColumnBasesModify>(fcb);
    }
    return forced_column_bases_;

} // ChartModify::forced_column_bases_modify

// ----------------------------------------------------------------------

ProjectionsModifyP ChartModify::projections_modify()
{
    if (!projections_)
        projections_ = std::make_shared<ProjectionsModify>(*this, main_->projections());
    return projections_;

} // ChartModify::projections_modify

// ----------------------------------------------------------------------

ProjectionModifyP ChartModify::projection_modify(size_t aProjectionNo)
{
    return projections_modify()->at(aProjectionNo);

} // ChartModify::projection_modify

// ----------------------------------------------------------------------

PlotSpecModifyP ChartModify::plot_spec_modify()
{
    if (!plot_spec_)
        plot_spec_ = std::make_shared<PlotSpecModify>(main_->plot_spec(), number_of_antigens());
    return plot_spec_;

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------

std::pair<optimization_status, ProjectionModifyP> ChartModify::relax(MinimumColumnBasis minimum_column_basis, size_t number_of_dimensions, bool dimension_annealing, optimization_options options, const PointIndexList& disconnect_points)
{
    const auto start = std::chrono::high_resolution_clock::now();
    const size_t start_num_dim = dimension_annealing && number_of_dimensions < 5 ? 5 : number_of_dimensions;
    auto projection = projections_modify()->new_from_scratch(start_num_dim, minimum_column_basis);
    projection->set_disconnected(disconnect_points);
    auto layout = projection->layout_modified();
    auto stress = acmacs::chart::stress_factory<double>(*projection, options.mult);
    projection->randomize_layout(options.max_distance_multiplier);
    auto status = acmacs::chart::optimize(options.method, stress, layout->data(), layout->data() + layout->size(), optimization_precision::rough);
    if (start_num_dim > number_of_dimensions) {
        acmacs::chart::dimension_annealing(options.method, projection->number_of_dimensions(), number_of_dimensions, layout->data(), layout->data() + layout->size());
        layout->change_number_of_dimensions(number_of_dimensions);
        stress.change_number_of_dimensions(number_of_dimensions);
        const auto status2 = acmacs::chart::optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
        status.number_of_iterations += status2.number_of_iterations;
        status.number_of_stress_calculations += status2.number_of_stress_calculations;
        status.termination_report = status2.termination_report;
        status.initial_stress = status2.initial_stress;
        status.final_stress = status2.final_stress;
    }
    projection->stress_ = status.final_stress;
    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    return {status, projection};

} // ChartModify::relax

// ----------------------------------------------------------------------

// void ChartModify::relax(size_t number_of_optimizations, MinimumColumnBasis minimum_column_basis, size_t number_of_dimensions, bool dimension_annealing, acmacs::chart::optimization_options options, bool report_stresses)
// {
//     const size_t start_num_dim = dimension_annealing && number_of_dimensions < 5 ? 5 : number_of_dimensions;
//     auto stress = acmacs::chart::stress_factory<double>(*this, start_num_dim, minimum_column_basis, options.mult, false);

// // #pragma omp parallel for default(none) firstprivate(stress) shared(start_num_dim,number_of_dimensions,dimension_annealing,options,report_stresses,number_of_optimizations,minimum_column_basis) num_threads(omp_get_max_threads()) schedule(static, 4)
//     for (size_t opt_no = 0 ; opt_no < number_of_optimizations; ++opt_no) {
//         auto projection = projections_modify()->new_from_scratch(start_num_dim, minimum_column_basis);
//         projection->randomize_layout(options.max_distance_multiplier);
//         auto layout = projection->layout_modified();
//         stress.change_number_of_dimensions(start_num_dim);
//         const auto status1 = acmacs::chart::optimize(options.method, stress, layout->data(), layout->data() + layout->size(), optimization_precision::rough);
//         if (start_num_dim > number_of_dimensions) {
//             acmacs::chart::dimension_annealing(options.method, projection->number_of_dimensions(), number_of_dimensions, layout->data(), layout->data() + layout->size());
//             layout->change_number_of_dimensions(number_of_dimensions);
//             stress.change_number_of_dimensions(number_of_dimensions);
//             const auto status2 = acmacs::chart::optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
//             projection->stress_ = status2.final_stress;
//         }
//         else {
//             projection->stress_ = status1.final_stress;
//         }
//         if (report_stresses)
//             std::cout << std::setw(3) << opt_no << ' ' << std::fixed << std::setprecision(4) << *projection->stress_ << '\n';
//     }

// } // ChartModify::relax

// ----------------------------------------------------------------------

LayoutRandomizerPlain ChartModify::make_randomizer(const Stress<double>& stress, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis) const
{
    ProjectionModifyNew prj(*this, number_of_dimensions, minimum_column_basis);
    auto rnd = prj.make_randomizer_plain_with_table_max_distance(1.0);
    prj.randomize_layout(rnd);
    acmacs::chart::optimize(optimization_method::alglib_cg_pca, stress, prj.layout_modified()->data(), prj.layout_modified()->size(), optimization_precision::very_rough);
    auto sq = [](double v) { return v*v; };
    const auto mm = prj.layout_modified()->minmax();
    const auto radius = std::sqrt(std::accumulate(mm.begin(), mm.end(), 0.0, [&sq](double sum, const auto& p) { return sum + sq(p.second - p.first); })) / 2.0;
    rnd.radius(radius);
    return rnd;

} // ChartModify::make_randomizer

// ----------------------------------------------------------------------

void ChartModify::relax(size_t number_of_optimizations, MinimumColumnBasis minimum_column_basis, size_t number_of_dimensions, bool dimension_annealing, acmacs::chart::optimization_options options, bool report_stresses)
{
    const size_t start_num_dim = dimension_annealing && number_of_dimensions < 5 ? 5 : number_of_dimensions;
    auto stress = acmacs::chart::stress_factory<double>(*this, start_num_dim, minimum_column_basis, options.mult, false);
    auto rnd = make_randomizer(stress, start_num_dim, minimum_column_basis);

    std::vector<std::shared_ptr<ProjectionModifyNew>> projections(number_of_optimizations);
    std::transform(projections.begin(), projections.end(), projections.begin(), [start_num_dim, minimum_column_basis, pp=projections_modify()](const auto&) { return pp->new_from_scratch(start_num_dim, minimum_column_basis); });

#pragma omp parallel for default(shared) firstprivate(stress) schedule(static, 4)
    for (size_t p_no = 0 ; p_no < projections.size(); ++p_no) {
        auto projection = projections[p_no];
        projection->randomize_layout(rnd);
        auto layout = projection->layout_modified();
        stress.change_number_of_dimensions(start_num_dim);
        const auto status1 = acmacs::chart::optimize(options.method, stress, layout->data(), layout->data() + layout->size(), optimization_precision::rough);
        if (start_num_dim > number_of_dimensions) {
            acmacs::chart::dimension_annealing(options.method, projection->number_of_dimensions(), number_of_dimensions, layout->data(), layout->data() + layout->size());
            layout->change_number_of_dimensions(number_of_dimensions);
            stress.change_number_of_dimensions(number_of_dimensions);
            const auto status2 = acmacs::chart::optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
            projection->stress_ = status2.final_stress;
        }
        else {
            projection->stress_ = status1.final_stress;
        }
        if (report_stresses) {
            auto sq = [](double v) { return v*v; };
            const auto mm = layout->minmax();
            const auto diameter = std::sqrt(sq(mm[0].second - mm[0].first) + sq(mm[1].second - mm[1].first));
            std::cout << std::setw(3) << p_no << ' ' << std::fixed << std::setprecision(4) << *projection->stress_ << " dia:" << diameter << '\n';
        }
    }

} // ChartModify::relax

// ----------------------------------------------------------------------

void ChartModify::remove_antigens(const ReverseSortedIndexes& indexes)
{
    antigens_modify()->remove(indexes);
    titers_modify()->remove_antigens(indexes);
    projections_modify()->remove_antigens(indexes);
    plot_spec_modify()->remove_antigens(indexes);

} // ChartModify::remove_antigens

// ----------------------------------------------------------------------

void ChartModify::remove_sera(const ReverseSortedIndexes& indexes)
{
    sera_modify()->remove(indexes);
    titers_modify()->remove_sera(indexes);
    projections_modify()->remove_sera(indexes, number_of_antigens());
    plot_spec_modify()->remove_sera(indexes);
    if (auto fcb = forced_column_bases_modify(); fcb)
        fcb->remove(indexes);

} // ChartModify::remove_sera

// ----------------------------------------------------------------------

constexpr const char* const sNames[] = {"Eddard Stark", "Robert Baratheon", "Jaime Lannister", "Catelyn Stark", "Cersei Lannister", "Daenerys Targaryen", "Jorah Mormont", "Petyr Baelish", "Viserys Targaryen", "Jon Snow", "Sansa Stark", "Arya Stark", "Robb Stark", "Theon Greyjoy", "Bran Stark", "Joffrey Baratheon", "Sandor Clegane", "Tyrion Lannister", "Khal Drogo", "Tywin Lannister", "Davos Seaworth", "Samwell Tarly", "Margaery Tyrell", "Stannis Baratheon", "Melisandre", "Jeor Mormont", "Bronn", "Varys", "Shae", "Ygritte", "Talisa Maegyr", "Gendry", "Tormund Giantsbane", "Gilly", "Brienne of Tarth", "Ramsay Bolton", "Ellaria Sand", "Daario Naharis", "Missandei", "Jaqen Hghar", "Tommen Baratheon", "Roose Bolton", "The High Sparrow", "Grand Maester Pycelle", "Meryn Trant", "Hodor", "Grenn", "Osha", "Rickon Stark", "Ros", "Gregor Clegane", "Janos Slynt", "Lancel Lannister", "Myrcella Baratheon", "Rodrik Cassel", "Maester Luwin", "Irri", "Doreah", "Kevan Lannister", "Barristan Selmy", "Rast", "Maester Aemon", "Pypar", "Alliser Thorne", "Othell Yarwyck", "Loras Tyrell", "Hot Pie", "Beric Dondarrion", "Podrick Payne", "Eddison Tollett", "Yara Greyjoy", "Selyse Florent", "Little Sam", "Grey Worm", "Qyburn", "Olenna Tyrell", "Shireen Baratheon", "Meera Reed", "Jojen Reed", "Thoros of Myr", "Yohn Royce", "Olly", "Mace Tyrell", "The Waif", "Bowen Marsh"};
constexpr size_t sNamesSize = sizeof(sNames) / sizeof(sNames[0]);

AntigenModifyP ChartModify::insert_antigen(size_t before)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, sNamesSize - 1);

    titers_modify()->modifiable_check();
    auto result = antigens_modify()->insert(before);
    result->name(sNames[dis(gen)]);
    titers_modify()->insert_antigen(before);
    projections_modify()->insert_antigen(before);
    plot_spec_modify()->insert_antigen(before);
    return result;

} // ChartModify::insert_antigen

// ----------------------------------------------------------------------

SerumModifyP ChartModify::insert_serum(size_t before)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, sNamesSize - 1);

    titers_modify()->modifiable_check();
    auto result = sera_modify()->insert(before);
    result->name(sNames[dis(gen)]);
    titers_modify()->insert_serum(before);
    projections_modify()->insert_serum(before, number_of_antigens());
    plot_spec_modify()->insert_serum(before);
    if (auto fcb = forced_column_bases_modify(); fcb) {
        std::cerr << "WARNING: inserting serum in the table having forced column bases, please set column basis for that serum\n";
        fcb->insert(before, 7.0); // all titers for the new serum are dont-care, there is no real column basis, use 1280 just to have something
    }
    return result;

} // ChartModify::insert_serum

// ----------------------------------------------------------------------

ChartNew::ChartNew(size_t number_of_antigens, size_t number_of_sera)
    : ChartModify(number_of_antigens, number_of_sera)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, sNamesSize - 1);

    auto& antigens = *antigens_modify();
      //size_t name_no = 0;
    std::string suffix = "";
    for (auto ag_no : acmacs::range(number_of_antigens)) {
        antigens.at(ag_no).name(sNames[dis(gen)] + suffix);
        // ++name_no;
        // if (name_no >= sNamesSize) {
        //     suffix += "A";
        //     name_no = 0;
        // }
    }

    auto& sera = *sera_modify();
    suffix = "";
    for (auto sr_no : acmacs::range(number_of_sera)) {
        sera.at(sr_no).name(sNames[dis(gen)] + suffix);
        // ++name_no;
        // if (name_no >= sNamesSize) {
        //     suffix += "S";
        //     name_no = 0;
        // }
    }

} // ChartNew::ChartNew

// ----------------------------------------------------------------------

InfoModify::InfoModify(InfoP main)
    : name_{main->name(Compute::No)},
      computed_name_{main->name(Compute::Yes)},
      virus_{main->virus(Compute::Yes)},
      virus_type_{main->virus_type(Compute::Yes)},
      subset_{main->subset(Compute::Yes)},
      assay_{main->assay(Compute::Yes)},
      lab_{main->lab(Compute::Yes)},
      rbc_species_{main->rbc_species(Compute::Yes)},
      date_{main->date(Compute::Yes)}
{
} // InfoModify::InfoModify

// ----------------------------------------------------------------------

AntigenModify::AntigenModify(AntigenP main)
    :
    name_{main->name()},
    date_{main->date()},
    passage_{main->passage()},
    lineage_{main->lineage()},
    reassortant_{main->reassortant()},
    annotations_{main->annotations()},
    lab_ids_{main->lab_ids()},
    clades_{main->clades()},
    reference_{main->reference()}
{
} // AntigenModify::AntigenModify

// ----------------------------------------------------------------------

SerumModify::SerumModify(SerumP main)
    :
    name_{main->name()},
    passage_{main->passage()},
    lineage_{main->lineage()},
    reassortant_{main->reassortant()},
    annotations_{main->annotations()},
    serum_id_{main->serum_id()},
    serum_species_{main->serum_species()},
    homologous_antigens_{main->homologous_antigens()}
{
} // SerumModify::SerumModify

// ----------------------------------------------------------------------

TitersModify::TitersModify(size_t number_of_antigens, size_t number_of_sera)
    : number_of_sera_{number_of_sera}, titers_{dense_t{number_of_antigens * number_of_sera, Titer{}}}
{
} // TitersModify::TitersModify

// ----------------------------------------------------------------------

TitersModify::TitersModify(TitersP main)
    : number_of_sera_{main->number_of_sera()},
      titers_{main->is_dense() ? titers_t{dense_t{}} : titers_t{sparse_t{}}}
{
      // Titers

    auto fill_titers = [this, &main](auto &titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
              // Dense ==================================================
            titers.resize(main->number_of_antigens() * this->number_of_sera_);
            for (const auto &entry : *main) { // order of iterations is not specified!
                // std::cerr << entry.antigen << ' ' << entry.serum << ' ' << entry.titer << '\n';
                titers[entry.antigen * this->number_of_sera_ + entry.serum] = entry.titer;
            }
        }
        else { // Sparse ==================================================
            titers.resize(main->number_of_antigens());
            for (const auto &entry : *main) { // order of iterations is not specified!
                titers[entry.antigen].emplace_back(entry.serum, entry.titer);
            }
              // sort entries by serum no
            for (auto& row : titers)
                std::sort(row.begin(), row.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
        }
    };

    std::visit(fill_titers, titers_);

      // Layers
    if (main->number_of_layers() > 0) {
        layers_.resize(main->number_of_layers());
        try {
            auto source = main->rjson_layers();
            for (const auto [layer_no, source_layer_v] : acmacs::enumerate(source)) {
                const rjson::array& source_layer = source_layer_v;
                auto& target = layers_[layer_no];
                target.resize(source_layer.size());
                for (const auto [ag_no, source_row] : acmacs::enumerate(source_layer)) {
                    for (const auto& entry : static_cast<const rjson::object&>(source_row)) {
                        target[ag_no].emplace_back(std::stoul(entry.first.str()), entry.second);
                    }
                }
                  // sort entries by serum no
                for (auto& row : target)
                    std::sort(row.begin(), row.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
            }
        }
        catch (data_not_available&) {
            for (size_t layer_no = 0; layer_no < main->number_of_layers(); ++layer_no) {
                auto& target = layers_[layer_no];
                target.resize(main->number_of_antigens());
                for (auto ag_no : acmacs::range(target.size())) {
                    for (auto sr_no : acmacs::range(number_of_sera_))
                        if (const auto titer = main->titer_of_layer(layer_no, ag_no, sr_no); !titer.is_dont_care() && !titer.is_invalid())
                            target[ag_no].emplace_back(sr_no, titer);
                }
            }
        }
    }

} // TitersModify::TitersModify

// ----------------------------------------------------------------------

inline acmacs::chart::Titer TitersModify::find_titer_for_serum(const sparse_row_t& aRow, size_t aSerumNo)
{
    if (aRow.empty())
        return {};
    if (const auto found = std::lower_bound(aRow.begin(), aRow.end(), aSerumNo,
                                            [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != aRow.end() && found->first == aSerumNo)
        return found->second;
    return {};

} // TitersModify::find_titer_for_serum

// ----------------------------------------------------------------------

inline acmacs::chart::Titer TitersModify::titer_in_sparse_t(const sparse_t& aSparse, size_t aAntigenNo, size_t aSerumNo)
{
    return find_titer_for_serum(aSparse[aAntigenNo], aSerumNo);

} // TitersModify::titer_in_sparse_t

// ----------------------------------------------------------------------

Titer TitersModify::titer(size_t aAntigenNo, size_t aSerumNo) const
{
    auto get = [this,aAntigenNo,aSerumNo](const auto& titers) -> Titer {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return titers[aAntigenNo * this->number_of_sera_ + aSerumNo];
        else
            return titer_in_sparse_t(titers, aAntigenNo, aSerumNo);
    };
    return std::visit(get, titers_);

} // TitersModify::titer

// ----------------------------------------------------------------------

Titer TitersModify::titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const
{
    return titer_in_sparse_t(layers_[aLayerNo], aAntigenNo, aSerumNo);

} // TitersModify::titer_of_layer

// ----------------------------------------------------------------------

std::vector<Titer> TitersModify::titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const
{
    if (layers_.empty())
        throw acmacs::chart::data_not_available("no layers");
    std::vector<Titer> result;
    for (const auto& layer: layers_) {
        if (const auto titer = find_titer_for_serum(layer[aAntigenNo], aSerumNo); !titer.is_dont_care())
            result.push_back(titer);
    }
    return result;

} // TitersModify::titers_for_layers

// ----------------------------------------------------------------------

size_t TitersModify::number_of_antigens() const
{
    auto num_ags = [this](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return titers.size() / this->number_of_sera_;
        else
            return titers.size();
    };
    return std::visit(num_ags, titers_);

} // TitersModify::number_of_antigens

// ----------------------------------------------------------------------

size_t TitersModify::number_of_non_dont_cares() const
{
    auto num_non_dont_cares = [](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return std::accumulate(titers.begin(), titers.end(), size_t{0}, [](size_t a, const auto& titer) -> size_t { return a + (titer.is_dont_care() ? size_t{0} : size_t{1}); });
        else
            return std::accumulate(titers.begin(), titers.end(), size_t{0}, [](size_t a, const auto& row) -> size_t { return a + row.size(); });
    };
    return std::visit(num_non_dont_cares, titers_);

} // TitersModify::number_of_non_dont_cares

// ----------------------------------------------------------------------

void TitersModify::titer(size_t aAntigenNo, size_t aSerumNo, const std::string& aTiter)
{
    modifiable_check();

    auto set_titer = [aAntigenNo,aSerumNo,&aTiter,this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            titers[aAntigenNo * this->number_of_sera_ + aSerumNo] = aTiter;
        }
        else {
            auto& row = titers[aAntigenNo];
            if (row.empty()) {
                row.emplace_back(aSerumNo, aTiter);
            }
            else {
                if (auto found = std::lower_bound(row.begin(), row.end(), aSerumNo, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end() && found->first == aSerumNo)
                    found->second = aTiter;
                else
                    row.emplace(found, aSerumNo, aTiter);
            }
        }
    };

    return std::visit(set_titer, titers_);

} // TitersModify::titer

// ----------------------------------------------------------------------

void TitersModify::dontcare_for_antigen(size_t aAntigenNo)
{
    modifiable_check();
    auto set_dontcare = [aAntigenNo, this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            std::fill_n(titers.begin() + static_cast<typename T::difference_type>(aAntigenNo * this->number_of_sera_), this->number_of_sera_, Titer{});
        }
        else {
            titers[aAntigenNo].clear();
        }
    };
    return std::visit(set_dontcare, titers_);

} // TitersModify::dontcare_for_antigen

// ----------------------------------------------------------------------

void TitersModify::dontcare_for_serum(size_t aSerumNo)
{
    modifiable_check();
    auto set_dontcare = [aSerumNo, this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            for (auto ag_no : acmacs::range(number_of_antigens()))
                titers[ag_no * this->number_of_sera_ + aSerumNo] = Titer{};
        }
        else {
            for (auto& row : titers) {
                if (auto found = std::lower_bound(row.begin(), row.end(), aSerumNo, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end() && found->first == aSerumNo)
                    row.erase(found);
            }
        }
    };
    return std::visit(set_dontcare, titers_);

} // TitersModify::dontcare_for_serum

// ----------------------------------------------------------------------

void TitersModify::multiply_by_for_antigen(size_t aAntigenNo, double multiply_by)
{
    modifiable_check();
    auto multiply = [aAntigenNo,multiply_by,this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            auto first = titers.begin() + static_cast<typename T::difference_type>(aAntigenNo * this->number_of_sera_);
            std::for_each(first, first + static_cast<typename T::difference_type>(this->number_of_sera_), [multiply_by](Titer& titer) { titer = titer.multiplied_by(multiply_by); });
        }
        else {
            std::for_each(titers[aAntigenNo].begin(), titers[aAntigenNo].end(), [multiply_by](sparse_entry_t& entry) { entry.second = entry.second.multiplied_by(multiply_by); });
        }
    };
    return std::visit(multiply, titers_);

} // TitersModify::multiply_by_for_antigen

// ----------------------------------------------------------------------

void TitersModify::multiply_by_for_serum(size_t aSerumNo, double multiply_by)
{
    modifiable_check();
    auto multiply = [aSerumNo, multiply_by, this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            for (auto ag_no : acmacs::range(number_of_antigens())) {
                auto& titer = titers[ag_no * this->number_of_sera_ + aSerumNo];
                titer = titer.multiplied_by(multiply_by);
            }
        }
        else {
            for (auto& row : titers) {
                if (auto found = std::lower_bound(row.begin(), row.end(), aSerumNo, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end() && found->first == aSerumNo)
                    found->second = found->second.multiplied_by(multiply_by);
            }
        }
    };
    return std::visit(multiply, titers_);

} // TitersModify::multiply_by_for_serum

// ----------------------------------------------------------------------

void TitersModify::remove_antigens(const ReverseSortedIndexes& indexes)
{
    auto do_remove_antigens_sparse = [&indexes](auto& titers) { acmacs::remove(indexes, titers); };

    auto do_remove_antigens_dense = [&indexes, this](auto& titers) {
        for (auto index : indexes) {
            const auto first = titers.begin() + static_cast<Indexes::difference_type>(index * this->number_of_sera_);
            titers.erase(first, first + static_cast<Indexes::difference_type>(this->number_of_sera_));
        }
    };

    auto do_remove_antigens = [&do_remove_antigens_sparse, &do_remove_antigens_dense](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            do_remove_antigens_dense(titers);
        else
            do_remove_antigens_sparse(titers);
    };

    std::visit(do_remove_antigens, titers_);
    for (auto& layer : layers_)
        do_remove_antigens_sparse(layer);

} // TitersModify::remove_antigens

// ----------------------------------------------------------------------

void TitersModify::insert_antigen(size_t before)
{
    modifiable_check();

    auto do_insert_antigen_sparse = [before](auto& titers) { titers.insert(titers.begin() + static_cast<Indexes::difference_type>(before), sparse_row_t{}); };

    auto do_insert_antigen_dense = [before, this](auto& titers) {
        titers.insert(titers.begin() + static_cast<Indexes::difference_type>(before * this->number_of_sera_), this->number_of_sera_, Titer{});
    };

    auto do_insert_antigen = [&do_insert_antigen_sparse, &do_insert_antigen_dense](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            do_insert_antigen_dense(titers);
        else
            do_insert_antigen_sparse(titers);
    };

    std::visit(do_insert_antigen, titers_);

} // TitersModify::insert_antigen

// ----------------------------------------------------------------------

void TitersModify::remove_sera(const ReverseSortedIndexes& indexes)
{
    auto do_remove_sera_sparse = [&indexes](auto& titers) {
        for (auto& row : titers) {
            for (auto index : indexes) {
                  // remove entry for index, then renumber entries for >index
                if (auto found = std::lower_bound(row.begin(), row.end(), index, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end()) {
                    if (found->first == index)
                        found = row.erase(found);
                    for (; found != row.end(); ++found)
                        --found->first;
                }
            }
        }
    };

    auto do_remove_sera_dense = [&indexes, this](auto& titers) {
        using diff_t = Indexes::difference_type;
        for (auto ag_no = static_cast<diff_t>(this->number_of_antigens()) - 1; ag_no >= 0; --ag_no) {
            for (auto index : indexes)
                titers.erase(titers.begin() + ag_no * static_cast<diff_t>(this->number_of_sera_) + static_cast<diff_t>(index));
        }
    };

    auto do_remove_sera = [&do_remove_sera_sparse, &do_remove_sera_dense](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            do_remove_sera_dense(titers);
        else
            do_remove_sera_sparse(titers);
    };

    std::visit(do_remove_sera, titers_);
    for (auto& layer : layers_)
        do_remove_sera_sparse(layer);

    number_of_sera_ -= indexes.size();

} // TitersModify::remove_sera

// ----------------------------------------------------------------------

void TitersModify::insert_serum(size_t before)
{
    modifiable_check();

    auto do_insert_serum_sparse = [before](auto& titers) {
        // renumber serum entries with indexes >= before
        for (auto& row : titers) {
            for (auto it = std::lower_bound(row.begin(), row.end(), before, [](const auto&e1, size_t sr_no) { return e1.first < sr_no; }); it != row.end(); ++it)
                ++it->first;
        }
    };

    auto do_insert_serum_dense = [before, this](auto& titers) {
        using diff_t = Indexes::difference_type;
        for (auto ag_no = static_cast<diff_t>(this->number_of_antigens()) - 1; ag_no >= 0; --ag_no)
            titers.insert(titers.begin() + ag_no * static_cast<diff_t>(this->number_of_sera_) + static_cast<diff_t>(before), Titer{});
    };

    auto do_insert_serum = [&do_insert_serum_sparse, &do_insert_serum_dense](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            do_insert_serum_dense(titers);
        else
            do_insert_serum_sparse(titers);
    };

    std::visit(do_insert_serum, titers_);
    ++number_of_sera_;

} // TitersModify::insert_serum

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionModify::clone(ChartModify& chart) const
{
    auto projection = std::make_shared<ProjectionModifyNew>(*this);
    chart.projections_modify()->add(projection);
    return projection;

} // ProjectionModify::clone

// ----------------------------------------------------------------------

void ProjectionModify::clone_from(const Projection& aSource)
{
    layout_ = std::make_shared<acmacs::Layout>(*aSource.layout());
    transformation_ = aSource.transformation();
    transformed_layout_.reset();
    set_forced_column_bases(aSource.forced_column_bases());

} // ProjectionModify::clone_from

// ----------------------------------------------------------------------

void ProjectionsModify::add(std::shared_ptr<ProjectionModify> projection)
{
    projections_.push_back(projection);
    projections_.back()->set_projection_no(projections_.size() - 1);

} // ProjectionsModify::add

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionsModify::new_from_scratch(size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis)
{
    auto projection = std::make_shared<ProjectionModifyNew>(chart(), number_of_dimensions, minimum_column_basis);
    add(projection);
    return projection;

} // ProjectionsModify::new_from_scratch

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionsModify::new_by_cloning(const ProjectionModify& source)
{
    auto projection = std::make_shared<ProjectionModifyNew>(source);
    add(projection);
    std::cerr << "new projection " << projection->projection_no() << '\n';
    return projection;

} // ProjectionsModify::new_by_cloning

// ----------------------------------------------------------------------

void ProjectionsModify::remove(size_t projection_no)
{
    if (projection_no >= projections_.size())
        throw invalid_data{"invalid projection number: " + std::to_string(projection_no)};
    projections_.erase(projections_.begin() + static_cast<decltype(projections_)::difference_type>(projection_no));
    set_projection_no();

} // ProjectionsModify::remove

// ----------------------------------------------------------------------

void ProjectionsModify::remove_all_except(size_t projection_no)
{
    if (projection_no >= projections_.size())
        throw invalid_data{"invalid projection number: " + std::to_string(projection_no)};
    projections_.erase(projections_.begin() + static_cast<decltype(projections_)::difference_type>(projection_no + 1), projections_.end());
    projections_.erase(projections_.begin(), projections_.begin() + static_cast<decltype(projections_)::difference_type>(projection_no));
    set_projection_no();

} // ProjectionsModify::remove_all_except

// ----------------------------------------------------------------------

void ProjectionModify::randomize_layout(LayoutRandomizer& randomizer)
{
    modify();
    for (double& val : *layout_modified())
        val = randomizer();

} // ProjectionModify::randomize_layout

// ----------------------------------------------------------------------

void ProjectionModify::randomize_layout(const PointIndexList& to_randomize, LayoutRandomizer& randomizer)
{
    modify();
    auto layout = layout_modified();
    for (auto point_no : to_randomize) {
        for (size_t dim_no = 0; dim_no < layout->number_of_dimensions(); ++dim_no)
            layout->set(point_no, dim_no, randomizer());
    }

} // ProjectionModify::randomize_layout

// ----------------------------------------------------------------------

LayoutRandomizerPlain ProjectionModify::make_randomizer_plain_with_table_max_distance(double max_distance_multiplier) const
{
    auto cb = forced_column_bases();
    if (!cb)
        cb = chart().column_bases(minimum_column_basis());
    const auto max_distance = chart().titers()->max_distance(*cb);
    // std::cerr << "max_distance: " << max_distance << '\n';
    return LayoutRandomizerPlain(max_distance * max_distance_multiplier);

} // ProjectionModify::make_randomizer_plain_with_table_max_distance

// ----------------------------------------------------------------------

void ProjectionModify::set_layout(const acmacs::Layout& layout, bool allow_size_change)
{
    modify();
    auto target_layout = layout_modified();
    if (!allow_size_change && layout.size() != target_layout->size())
        throw invalid_data("ProjectionModify::set_layout(const acmacs::Layout&): wrong layout size");
    *target_layout = layout;

} // ProjectionModify::set_layout

// ----------------------------------------------------------------------

void ProjectionModify::set_layout(const acmacs::LayoutInterface& layout)
{
    modify();
    new_layout(layout.number_of_points(), layout.number_of_dimensions());
    for (auto point_no : acmacs::range(layout.number_of_points()))
        layout_->set(point_no, layout.get(point_no));

} // ProjectionModify::set_layout

// ----------------------------------------------------------------------

void ProjectionModifyNew::connect(const PointIndexList& to_connect)
{
    for (auto point_no: to_connect) {
        const auto found = std::find(disconnected_.begin(), disconnected_.end(), point_no);
        if (found == disconnected_.end())
            throw invalid_data{"Point was not disconnected: " + std::to_string(point_no) + ": cannot connect it"};
        disconnected_.erase(found);
    }

} // ProjectionModifyNew::connect

// ----------------------------------------------------------------------

PlotSpecModify::PlotSpecModify(size_t number_of_antigens, size_t number_of_sera)
    : main_{nullptr}, number_of_antigens_(number_of_antigens), modified_{true}, styles_(number_of_antigens + number_of_sera, PointStyle{})
{
    for (size_t point_no : acmacs::range(number_of_antigens)) {
        styles_[point_no].fill = "green";
    }
    for (size_t point_no : acmacs::range(number_of_antigens, number_of_antigens + number_of_sera)) {
        styles_[point_no].shape = PointShape::Box;
    }
    drawing_order_.fill_if_empty(number_of_antigens + number_of_sera);

} // PlotSpecModify::PlotSpecModify

// ----------------------------------------------------------------------

void PlotSpecModify::remove_antigens(const ReverseSortedIndexes& indexes)
{
    modify();
    acmacs::remove(indexes, styles_);
    drawing_order_.remove_points(indexes);
    number_of_antigens_ -= indexes.size();

} // PlotSpecModify::remove_antigens

// ----------------------------------------------------------------------

void PlotSpecModify::remove_sera(const ReverseSortedIndexes& indexes)
{
    modify();
    acmacs::remove(indexes, styles_, number_of_antigens_);
    drawing_order_.remove_points(indexes, number_of_antigens_);

} // PlotSpecModify::remove_sera

// ----------------------------------------------------------------------

void PlotSpecModify::insert_antigen(size_t before)
{
    modify();
    styles_.insert(styles_.begin() + static_cast<decltype(styles_)::difference_type>(before), PointStyle{});
    drawing_order_.insert(before);
    ++number_of_antigens_;

} // PlotSpecModify::insert_antigen

// ----------------------------------------------------------------------

void PlotSpecModify::insert_serum(size_t before)
{
    modify();
    styles_.insert(styles_.begin() + static_cast<decltype(styles_)::difference_type>(before + number_of_antigens_), PointStyle{});
    drawing_order_.insert(before + number_of_antigens_);

} // PlotSpecModify::insert_serum

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
