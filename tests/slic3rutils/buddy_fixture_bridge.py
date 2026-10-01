"""Opt-in adapter for the separately checked-out Buddy virtual-printer fixtures."""
import importlib.util
import os
from pathlib import Path
import sys

root = Path(os.environ["BUDDY_SOURCE_PATH"]).resolve()
package = root / "tests" / "integration"
spec = importlib.util.spec_from_file_location("buddy_integration", package / "__init__.py",
                                            submodule_search_locations=[str(package)])
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)
pytest_plugins = ["buddy_integration.conftest"]
