package com.example.aclrecoveryanalyzer.ui.navigation

import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Bluetooth
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Sensors
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.navigation.NavDestination.Companion.hasRoute
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.example.aclrecoveryanalyzer.ui.device.DeviceScreen
import com.example.aclrecoveryanalyzer.ui.history.HistoryScreen
import com.example.aclrecoveryanalyzer.ui.live.LiveSessionScreen
import com.example.aclrecoveryanalyzer.ui.settings.SettingsScreen
import kotlinx.serialization.Serializable

// Type-safe route definitions
@Serializable object LiveRoute
@Serializable object HistoryRoute
@Serializable object DeviceRoute
@Serializable object SettingsRoute

data class TopLevelRoute<T : Any>(
    val name: String,
    val route: T,
    val icon: ImageVector
)

val topLevelRoutes = listOf(
    TopLevelRoute("Live", LiveRoute, Icons.Default.Sensors),
    TopLevelRoute("History", HistoryRoute, Icons.Default.History),
    TopLevelRoute("Device", DeviceRoute, Icons.Default.Bluetooth),
    TopLevelRoute("Settings", SettingsRoute, Icons.Default.Settings)
)

@Composable
fun AppNavHost() {
    val navController = rememberNavController()
    val navBackStackEntry by navController.currentBackStackEntryAsState()
    val currentDestination = navBackStackEntry?.destination

    Scaffold(
        bottomBar = {
            NavigationBar {
                topLevelRoutes.forEach { topLevelRoute ->
                    NavigationBarItem(
                        icon = { Icon(topLevelRoute.icon, contentDescription = topLevelRoute.name) },
                        label = { Text(topLevelRoute.name) },
                        selected = currentDestination?.hasRoute(topLevelRoute.route::class) == true,
                        onClick = {
                            navController.navigate(topLevelRoute.route) {
                                popUpTo(navController.graph.findStartDestination().id) {
                                    saveState = true
                                }
                                launchSingleTop = true
                                restoreState = true
                            }
                        }
                    )
                }
            }
        }
    ) { innerPadding ->
        NavHost(
            navController = navController,
            startDestination = LiveRoute,
            modifier = Modifier.padding(innerPadding)
        ) {
            composable<LiveRoute> { LiveSessionScreen() }
            composable<HistoryRoute> { HistoryScreen() }
            composable<DeviceRoute> { DeviceScreen() }
            composable<SettingsRoute> { SettingsScreen() }
        }
    }
}
