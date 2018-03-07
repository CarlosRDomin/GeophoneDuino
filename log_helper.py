import coloredlogs
import verboselogs


# Create a colored logger so it's easy to skim console messages
logger = verboselogs.VerboseLogger(__name__)
coloredlogs.install(level="DEBUG", fmt="%(asctime)s.%(msecs)03d %(levelname)-8s %(message)s", field_styles={"asctime": {"color": "cyan", "bold": True}, "levelname": {"bold": True}})