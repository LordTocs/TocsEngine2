#include "Component.h"


namespace tocs {
namespace engine{

core::static_storage<std::vector<std::pair<std::type_index, std::function<base_component_storage *()>>>> all_component_storage::storage_factories;


}
}